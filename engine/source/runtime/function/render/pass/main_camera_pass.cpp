#include <map>
#include <stdexcept>
#include <array>
#include <iostream>
#include "main_camera_pass.h"
#include "function/render/render_resource.h"
#include "cascade_shadow_map_pass.h"
#include "function/global/engine_global_context.h"
#include "function/render/scene_manager.h"
#include "scene_frag.h"
#include "scene_vert.h"

namespace MW {
    extern VkDescriptorSetLayout descriptorSetLayoutImage;

    void MainCameraPass::initialize(const RenderPassInitInfo *init_info) {
        PassBase::initialize(init_info);
        shadowMapPass = std::make_shared<CascadeShadowMapPass>();
        shadowMapPass->initialize(init_info);

        loadModel();
        createRenderPass();
        createUniformBuffer();
        createDescriptorSets();
        createPipelines();
        createSwapchainFramebuffers();
    }

    void MainCameraPass::createRenderPass() {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = device->getDepthImageInfo().depthImageFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = device->getSwapchainInfo().imageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 1;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstSubpass = 0;
        dependency.dstStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {depthAttachment, colorAttachment};
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        device->CreateRenderPass(&renderPassInfo, &framebuffer.renderPass);
    }

    void MainCameraPass::createUniformBuffer() {
        device->CreateBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                cameraUniformBuffer,
                sizeof(renderResource->cameraObject),
                &renderResource->cameraObject);
        device->MapMemory(cameraUniformBuffer);

        updateCamera();
    }

    void MainCameraPass::createPipelines() {
        pipelines.resize(1);
        auto vertShaderModule = device->CreateShaderModule(SCENE_VERT);
        auto fragShaderModule = device->CreateShaderModule(SCENE_FRAG);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        std::vector<VertexComponent> components{VertexComponent::Position, VertexComponent::Normal, VertexComponent::UV,
                                                VertexComponent::Color, VertexComponent::Tangent};

//        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//        vertexInputInfo.vertexBindingDescriptionCount = 0;
//        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
        depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
        depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;

        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        VkPushConstantRange pushConstantRange = CreatePushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4),
                                                                        0);

        std::vector<VkDescriptorSetLayout> layouts{descriptors[0].layout, descriptorSetLayoutImage};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        device->CreatePipelineLayout(&pipelineLayoutInfo, &pipelines[0].layout);

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = gltfVertex::getPipelineVertexInputState(components);
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.pDepthStencilState = &depthStencilCreateInfo;
        pipelineInfo.layout = pipelines[0].layout;
        pipelineInfo.renderPass = framebuffer.renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        device->CreateGraphicsPipelines(&pipelineInfo, &pipelines[0].pipeline);

        device->DestroyShaderModule(fragShaderModule);
        device->DestroyShaderModule(vertShaderModule);
    }

    void MainCameraPass::createDescriptorSets() {
        descriptors.resize(1);
        {
            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding = 0;
            layoutBinding.descriptorCount = 1;
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings = &layoutBinding;
            device->CreateDescriptorSetLayout(&layoutInfo, &descriptors[0].layout);
        }

        {
            device->CreateDescriptorSet(1, descriptors[0].layout, descriptors[0].descriptorSet);

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = cameraUniformBuffer.buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(CameraObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptors[0].descriptorSet;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            device->UpdateDescriptorSets(1, &descriptorWrite);
        }

    }

    void MainCameraPass::createSwapchainFramebuffers() {
        swapChainFramebuffers.resize(device->imageCount());
        for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                    device->getDepthImageInfo().depthImageView,
                    device->getSwapchainInfo().imageViews[i]};

            VkExtent2D swapChainExtent = device->getSwapchainInfo().extent;
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = framebuffer.renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            device->CreateFramebuffer(
                    &framebufferInfo,
                    &swapChainFramebuffers[i]);
        }
    }

    void MainCameraPass::draw() {
        shadowMapPass->drawDepth();
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = framebuffer.renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[device->currentImageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = device->getSwapchainInfo().extent;

        std::array<VkClearValue, 2> clearColors;
        clearColors[0] = {1.0f, 0};
        clearColors[1] = {{0.0f, 0.0f, 0.2f, 1.0f}};
        renderPassInfo.clearValueCount = clearColors.size();
        renderPassInfo.pClearValues = clearColors.data();

        vkCmdBeginRenderPass(device->getCurrentCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) device->getSwapchainInfo().extent.width;
        viewport.height = (float) device->getSwapchainInfo().extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(device->getCurrentCommandBuffer(), 0, 1, &viewport);

        vkCmdSetScissor(device->getCurrentCommandBuffer(), 0, 1, device->getSwapchainInfo().scissor);

        shadowMapPass->draw();
        // Bind scene matrices descriptor to set 0
//        vkCmdBindPipeline(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].pipeline);
//
//        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout,
//                                0, 1, &descriptors[0].descriptorSet, 0, nullptr);
//
//        engineGlobalContext.getScene()->draw(device->getCurrentCommandBuffer(), RenderFlags::BindImages,
//                                               pipelines[0].layout, 1);
//        scene.draw(device->getCurrentCommandBuffer(), RenderFlags::BindImages, pipelines[0].layout, 1);
//        vkCmdDraw(device->getCurrentCommandBuffer(), 3, 1, 0, 0);

        vkCmdEndRenderPass(device->getCurrentCommandBuffer());
    }

    void MainCameraPass::updateAfterFramebufferRecreate() {
        for (size_t i = 0; i < framebuffer.attachments.size(); i++) {
            device->DestroyImage(framebuffer.attachments[i].image);
            device->DestroyImageView(framebuffer.attachments[i].view);
            device->FreeMemory(framebuffer.attachments[i].mem);
        }

        for (auto framebuffer: swapChainFramebuffers) {
            device->DestroyFramebuffer(framebuffer);
        }

        createSwapchainFramebuffers();
    }

    void MainCameraPass::updateCamera() {
        memcpy(cameraUniformBuffer.mapped, &renderResource->cameraObject, sizeof(renderResource->cameraObject));
    }

    void MainCameraPass::preparePassData() {
        updateCamera();
        shadowMapPass->preparePassData();
    }

    void MainCameraPass::loadModel() {
        const uint32_t glTFLoadingFlags =
//                FileLoadingFlags::PreTransformVertices | FileLoadingFlags::PreMultiplyVertexColors |
                FileLoadingFlags::PreTransformVertices |
                FileLoadingFlags::FlipY;
        scene.loadFromFile(getAssetPath() + "models/sponza/sponza.gltf", device.get(), glTFLoadingFlags);
    }
}