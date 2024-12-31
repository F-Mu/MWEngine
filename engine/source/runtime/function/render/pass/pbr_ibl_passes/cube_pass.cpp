#include "cube_pass.h"
#include <array>
#include "function/global/engine_global_context.h"
#include "function/render/scene_manager.h"
#include "cube_vert.h"

namespace MW {
    void CubePass::initialize(const RenderPassInitInfo *info) {
        PassBase::initialize(info);

        const auto *_info = static_cast<const CubePassInitInfo *>(info);
        useEnvironmentCube = _info->useEnvironmentCube;
        pushBlockBuffer = _info->pushBlock;
        pushBlockBuffer.size += sizeof(pushBlockBuffer.pushBlock.mvp);
        fragShader = _info->fragShader;
        imageDim = _info->imageDim;
        numMips = static_cast<uint32_t>(floor(log2(imageDim))) + 1;
        type = _info->type;
        loadCube();
        createCubeMap();
        createRenderPass();
        createDescriptorSets();
        createFramebuffers();
        createPipelines();
    }

    void CubePass::clean() {
        for (auto &pipeline: pipelines) {
            device->DestroyPipeline(pipeline.pipeline);
            device->DestroyPipelineLayout(pipeline.layout);
        }
        for (size_t i = 0; i < framebuffer.attachments.size(); i++) {
            device->DestroyImage(framebuffer.attachments[i].image);
            device->DestroyImageView(framebuffer.attachments[i].view);
            device->FreeMemory(framebuffer.attachments[i].mem);
        }
        device->DestroyFramebuffer(framebuffer.framebuffer);
        for (auto &descriptor: descriptors) {
            device->DestroyDescriptorSetLayout(descriptor.layout);
        }
        device->DestroyRenderPass(framebuffer.renderPass);
        cubeMap.destroy(device);
        cube.clean();
        PassBase::clean();
    }

    void CubePass::draw() {
        if (executed) return;
        executed = true;
        // Render

        VkClearValue clearValues[1];
        clearValues[0].color = {{0.0f, 0.0f, 0.2f, 0.0f}};

        VkRenderPassBeginInfo renderPassBeginInfo = CreateRenderPassBeginInfo();
        // Reuse render pass from example pass
        renderPassBeginInfo.renderPass = framebuffer.renderPass;
        renderPassBeginInfo.framebuffer = framebuffer.framebuffer;
        renderPassBeginInfo.renderArea.extent.width = imageDim;
        renderPassBeginInfo.renderArea.extent.height = imageDim;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = clearValues;

        std::vector<glm::mat4> matrices = {
                // POSITIVE_X
                glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                            glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_X
                glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                            glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // POSITIVE_Y
                glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_Y
                glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // POSITIVE_Z
                glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_Z
                glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        };

        void *pushData = operator new(pushBlockBuffer.size);
        VkCommandBuffer cmdBuf = device->beginSingleTimeCommands();
        VkViewport viewport = CreateViewport((float) imageDim, (float) imageDim, 0.0f, 1.0f);
        VkRect2D scissor = CreateRect2D(imageDim, imageDim, 0, 0);

        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = numMips;
        subresourceRange.layerCount = 6;

        // Change image layout for all cubemap faces to transfer destination
        device->transitionImageLayout(
                cmdBuf,
                cubeMap.image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                subresourceRange);

        for (uint32_t m = 0; m < numMips; m++) {
            updatePushBlock(m);
            for (uint32_t f = 0; f < 6; f++) {
                viewport.width = static_cast<float>(imageDim * std::pow(0.5f, m));
                viewport.height = static_cast<float>(imageDim * std::pow(0.5f, m));
                vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

                // Render scene from cube face's point of view
                vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Update shader push constant block
                pushBlockBuffer.pushBlock.mvp =
                        glm::perspective((float) (M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

                memcpy(pushData, &pushBlockBuffer.pushBlock.mvp, sizeof(pushBlockBuffer.pushBlock.mvp));
                memcpy((char *) pushData + sizeof(pushBlockBuffer.pushBlock.mvp), pushBlockBuffer.pushBlock.data,
                       pushBlockBuffer.size - sizeof(pushBlockBuffer.pushBlock.mvp));
                vkCmdPushConstants(cmdBuf, pipelines[0].layout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                   pushBlockBuffer.size, pushData);

                vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].pipeline);
                vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout, 0, 1,
                                        &descriptors[0].descriptorSet,
                                        0, NULL);

                cube.draw(cmdBuf);

                vkCmdEndRenderPass(cmdBuf);

                device->transitionImageLayout(
                        cmdBuf,
                        framebuffer.attachments[0].image,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                // Copy region for transfer from framebuffer to cube face
                VkImageCopy copyRegion = {};

                copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.srcSubresource.baseArrayLayer = 0;
                copyRegion.srcSubresource.mipLevel = 0;
                copyRegion.srcSubresource.layerCount = 1;
                copyRegion.srcOffset = {0, 0, 0};

                copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.dstSubresource.baseArrayLayer = f;
                copyRegion.dstSubresource.mipLevel = m;
                copyRegion.dstSubresource.layerCount = 1;
                copyRegion.dstOffset = {0, 0, 0};

                copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
                copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
                copyRegion.extent.depth = 1;

                vkCmdCopyImage(
                        cmdBuf,
                        framebuffer.attachments[0].image,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        cubeMap.image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1,
                        &copyRegion);

                // Transform framebuffer color attachment back
                device->transitionImageLayout(
                        cmdBuf,
                        framebuffer.attachments[0].image,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            }
        }

        device->transitionImageLayout(
                cmdBuf,
                cubeMap.image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                subresourceRange);

        device->endSingleTimeCommands(cmdBuf);
        operator delete(pushData);
    }

    void CubePass::createRenderPass() {
        // FB, Att, RP, Pipe, etc.
        VkAttachmentDescription attDesc = {};
        // Color attachment
        attDesc.format = cubeMapFormat;
        attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Renderpass
        VkRenderPassCreateInfo renderPassCI = CreateRenderPassCreateInfo();
        renderPassCI.attachmentCount = 1;
        renderPassCI.pAttachments = &attDesc;
        renderPassCI.subpassCount = 1;
        renderPassCI.pSubpasses = &subpassDescription;
        renderPassCI.dependencyCount = dependencies.size();
        renderPassCI.pDependencies = dependencies.data();
        device->CreateRenderPass(&renderPassCI, &framebuffer.renderPass);
    }

    void CubePass::createDescriptorSets() {
        if (!useEnvironmentCube)return;
        descriptors.resize(1);
        // Descriptors
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCI = CreateDescriptorSetLayoutCreateInfo(setLayoutBindings);
        device->CreateDescriptorSetLayout(&descriptorsetlayoutCI, &descriptors[0].layout);
        device->CreateDescriptorSet(1, descriptors[0].layout, descriptors[0].descriptorSet);
        VkWriteDescriptorSet writeDescriptorSet = CreateWriteDescriptorSet(descriptors[0].descriptorSet,
                                                                           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                                                           &engineGlobalContext.getScene()->getSkyBox()->descriptor);
        device->UpdateDescriptorSets(1, &writeDescriptorSet);
    }

    void CubePass::createFramebuffers() {
        framebuffer.attachments.resize(1);

        VkImageCreateInfo imageCreateInfo = CreateImageCreateInfo();
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = cubeMapFormat;
        imageCreateInfo.extent.width = imageDim;
        imageCreateInfo.extent.height = imageDim;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        device->CreateImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                    framebuffer.attachments[0].image, framebuffer.attachments[0].mem);

        VkImageViewCreateInfo colorImageView = CreateImageViewCreateInfo();
        colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorImageView.format = cubeMapFormat;
        colorImageView.flags = 0;
        colorImageView.subresourceRange = {};
        colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorImageView.subresourceRange.baseMipLevel = 0;
        colorImageView.subresourceRange.levelCount = 1;
        colorImageView.subresourceRange.baseArrayLayer = 0;
        colorImageView.subresourceRange.layerCount = 1;
        colorImageView.image = framebuffer.attachments[0].image;
        device->CreateImageView(&colorImageView, &framebuffer.attachments[0].view);

        VkFramebufferCreateInfo framebufferCreateInfo = CreateFramebufferCreateInfo();
        framebufferCreateInfo.renderPass = framebuffer.renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &framebuffer.attachments[0].view;
        framebufferCreateInfo.width = imageDim;
        framebufferCreateInfo.height = imageDim;
        framebufferCreateInfo.layers = 1;
        device->CreateFramebuffer(&framebufferCreateInfo, &framebuffer.framebuffer);


        VkCommandBuffer layoutCmd = device->beginSingleTimeCommands();
        device->transitionImageLayout(
                layoutCmd,
                framebuffer.attachments[0].image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        device->endSingleTimeCommands(layoutCmd);
    }

    void CubePass::createPipelines() {
        pipelines.resize(1);
        std::vector<VkPushConstantRange> pushConstantRanges = {
                CreatePushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, pushBlockBuffer.size,
                                        0),
        };
        VkPipelineLayoutCreateInfo pipelineLayoutCI = CreatePipelineLayoutCreateInfo(&descriptors[0].layout);
        pipelineLayoutCI.pushConstantRangeCount = 1;
        pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();
        device->CreatePipelineLayout(&pipelineLayoutCI, &pipelines[0].layout);
        // Pipeline
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = CreatePipelineInputAssemblyStateCreateInfo(
                VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState = CreatePipelineRasterizationStateCreateInfo(
                VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState blendAttachmentState = CreatePipelineColorBlendAttachmentState(0xf,
                                                                                                           VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = CreatePipelineColorBlendStateCreateInfo(1,
                                                                                                      &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState = CreatePipelineDepthStencilStateCreateInfo(VK_FALSE,
                                                                                                            VK_FALSE,
                                                                                                            VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = CreatePipelineViewportStateCreateInfo(1, 1);
        VkPipelineMultisampleStateCreateInfo multisampleState = CreatePipelineMultisampleStateCreateInfo(
                VK_SAMPLE_COUNT_1_BIT);
        std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState = CreatePipelineDynamicStateCreateInfo(dynamicStateEnables);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
        auto vertModule = device->CreateShaderModule(CUBE_VERT);
        auto fragModule = device->CreateShaderModule(*fragShader);
        shaderStages[0] = loadShader(vertModule, VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = loadShader(fragModule, VK_SHADER_STAGE_FRAGMENT_BIT);

        VkGraphicsPipelineCreateInfo pipelineCI = CreatePipelineCreateInfo(pipelines[0].layout, framebuffer.renderPass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = 2;
        pipelineCI.pStages = shaderStages.data();
        pipelineCI.pVertexInputState = gltfVertex::getPipelineVertexInputState(
                {VertexComponent::Position});
        device->CreateGraphicsPipelines(&pipelineCI, &pipelines[0].pipeline);
        device->DestroyShaderModule(vertModule);
        device->DestroyShaderModule(fragModule);
    }

    void CubePass::createCubeMap() {
        VkImageCreateInfo imageCI = CreateImageCreateInfo();

        const VkFormat format = cubeMapFormat;
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.format = format;
        imageCI.extent.width = imageDim;
        imageCI.extent.height = imageDim;
        imageCI.extent.depth = 1;
        imageCI.mipLevels = numMips;
        imageCI.arrayLayers = 6;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        device->CreateImageWithInfo(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cubeMap.image, cubeMap.deviceMemory);

        VkImageViewCreateInfo viewCI = CreateImageViewCreateInfo();
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewCI.format = format;
        viewCI.subresourceRange = {};
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.levelCount = numMips;
        viewCI.subresourceRange.layerCount = 6;
        viewCI.image = cubeMap.image;
        device->CreateImageView(&viewCI, &cubeMap.view);
        // Sampler
        VkSamplerCreateInfo samplerCI = CreateSamplerCreateInfo();
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.minLod = 0.0f;
        samplerCI.maxLod = static_cast<float>(numMips);
        samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        device->CreateSampler(&samplerCI, &cubeMap.sampler);

        cubeMap.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        cubeMap.updateDescriptor();
    }

    void CubePass::loadCube() {
        uint32_t glTFLoadingFlags = FileLoadingFlags::PreTransformVertices | FileLoadingFlags::FlipY;
        cube.loadFromFile(getAssetPath() + "models/cube.gltf", device.get(), glTFLoadingFlags);
    }

    void CubePass::updatePushBlock(uint32_t mip) {
        switch (type) {
            case prefilter_cube_pass: {
                auto *block = static_cast<prefilterPushBlock *>(pushBlockBuffer.pushBlock.data);
                block->roughness = (float) mip / (float) (numMips - 1);
                break;
            }
            case irradiance_cube_pass: {
                break;
            }
            default:
                break;
        }
    }
}