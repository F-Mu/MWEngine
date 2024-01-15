#include "lut_pass.h"
#include "deferred_vert.h"
#include "brdf_lut_frag.h"
#include <array>

namespace MW {

    void LutPass::initialize(const RenderPassInitInfo *info) {
        PassBase::initialize(info);
        const auto *_info = static_cast<const LutPassInitInfo *>(info);
        imageDim = _info->imageDim;
        createLutTexture();
        createRenderPass();
        createFramebuffers();
        createPipelines();
    }

    void LutPass::draw() {
        if (executed) return;
        executed = true;
        // Render
        VkClearValue clearValues[1];
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

        VkRenderPassBeginInfo renderPassBeginInfo = CreateRenderPassBeginInfo();
        renderPassBeginInfo.renderPass = framebuffer.renderPass;
        renderPassBeginInfo.renderArea.extent.width = imageDim;
        renderPassBeginInfo.renderArea.extent.height = imageDim;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = clearValues;
        renderPassBeginInfo.framebuffer = framebuffer.framebuffer;

        VkCommandBuffer cmdBuf = device->beginSingleTimeCommands();
        vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport viewport = CreateViewport((float) imageDim, (float) imageDim, 0.0f, 1.0f);
        VkRect2D scissor = CreateRect2D(imageDim, imageDim, 0, 0);
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].pipeline);
        vkCmdDraw(cmdBuf, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmdBuf);
        device->endSingleTimeCommands(cmdBuf);
    }

    void LutPass::createRenderPass() {
        // FB, Att, RP, Pipe, etc.
        VkAttachmentDescription attDesc = {};
        // Color attachment
        attDesc.format = lutFormat;
        attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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

        // Create the actual renderpass
        VkRenderPassCreateInfo renderPassCI = CreateRenderPassCreateInfo();
        renderPassCI.attachmentCount = 1;
        renderPassCI.pAttachments = &attDesc;
        renderPassCI.subpassCount = 1;
        renderPassCI.pSubpasses = &subpassDescription;
        renderPassCI.dependencyCount = 2;
        renderPassCI.pDependencies = dependencies.data();

        device->CreateRenderPass(&renderPassCI, &framebuffer.renderPass);
    }

    void LutPass::createFramebuffers() {
        VkFramebufferCreateInfo framebufferCI = CreateFramebufferCreateInfo();
        framebufferCI.renderPass = framebuffer.renderPass;
        framebufferCI.attachmentCount = 1;
        framebufferCI.pAttachments = &lutTexture.view;
        framebufferCI.width = imageDim;
        framebufferCI.height = imageDim;
        framebufferCI.layers = 1;
        device->CreateFramebuffer(&framebufferCI, &framebuffer.framebuffer);
    }

    void LutPass::createPipelines() {
        pipelines.resize(1);
        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutCI = CreatePipelineLayoutCreateInfo(nullptr, 0);
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
        VkPipelineVertexInputStateCreateInfo emptyInputState = CreatePipelineVertexInputStateCreateInfo();
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
        shaderStages[0] = loadShader(device->CreateShaderModule(DEFERRED_VERT), VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = loadShader(device->CreateShaderModule(BRDF_LUT_FRAG), VK_SHADER_STAGE_FRAGMENT_BIT);

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
        pipelineCI.pVertexInputState = &emptyInputState;
        device->CreateGraphicsPipelines(&pipelineCI, &pipelines[0].pipeline);
    }

    void LutPass::createLutTexture() {
        // Image
        VkImageCreateInfo imageCI = CreateImageCreateInfo();
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.format = lutFormat;
        imageCI.extent.width = imageDim;
        imageCI.extent.height = imageDim;
        imageCI.extent.depth = 1;
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 1;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        device->CreateImageWithInfo(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, lutTexture.image,
                                    lutTexture.deviceMemory);
        // Image view
        VkImageViewCreateInfo viewCI = CreateImageViewCreateInfo();
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format = lutFormat;
        viewCI.subresourceRange = {};
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.levelCount = 1;
        viewCI.subresourceRange.layerCount = 1;
        viewCI.image = lutTexture.image;
        device->CreateImageView(&viewCI, &lutTexture.view);
        // Sampler
        VkSamplerCreateInfo samplerCI = CreateSamplerCreateInfo();
        samplerCI.magFilter = VK_FILTER_LINEAR;
        samplerCI.minFilter = VK_FILTER_LINEAR;
        samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCI.minLod = 0.0f;
        samplerCI.maxLod = 1.0f;
        samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        device->CreateSampler(&samplerCI, &lutTexture.sampler);
        lutTexture.updateDescriptor();
    }
}