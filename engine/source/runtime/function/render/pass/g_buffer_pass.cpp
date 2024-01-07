#include "g_buffer_pass.h"
#include "function/global/engine_global_context.h"
#include "function/render/render_system.h"
#include "function/render/render_camera.h"
#include "function/render/render_resource.h"
#include "function/render/render_model.h"
#include "function/global/engine_global_context.h"
#include "function/render/scene_manager.h"
#include "cascade_shadow_map_vert.h"
#include "cascade_shadow_map_frag.h"
#include "scene_gbuffer_vert.h"
#include "scene_gbuffer_frag.h"
#include "depthpass_vert.h"

namespace MW {
    extern VkDescriptorSetLayout descriptorSetLayoutImage;

    void GBufferPass::initialize(const RenderPassInitInfo *info) {
        PassBase::initialize(info);
        const auto *_info = static_cast<const GBufferPassInitInfo *>(info);
        framebuffer.renderPass = *_info->renderPass;
        createUniformBuffer();
        createDescriptorSets();
        createPipelines();
        createFramebuffers();
    }

    void GBufferPass::preparePassData() {
        gBufferCameraProject.projView = renderResource->cameraObject.projViewMatrix;
        memcpy(cameraUboBuffer.mapped, &gBufferCameraProject, sizeof(gBufferCameraProject));
    }

    void GBufferPass::createUniformBuffer() {
        device->CreateBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                cameraUboBuffer,
                sizeof(gBufferCameraProject));
        device->MapMemory(cameraUboBuffer);
    }

    void GBufferPass::createDescriptorSets() {
        descriptors.resize(1);
        std::vector<VkDescriptorSetLayoutBinding> binding = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pBindings = binding.data();
        descriptorSetLayoutCreateInfo.bindingCount = binding.size();
        device->CreateDescriptorSetLayout(&descriptorSetLayoutCreateInfo, &descriptors[0].layout);
        device->CreateDescriptorSet(1, descriptors[0].layout, descriptors[0].descriptorSet);

        VkDescriptorBufferInfo cameraUboBufferInfo{};
        cameraUboBufferInfo.buffer = cameraUboBuffer.buffer;
        cameraUboBufferInfo.offset = 0;
        cameraUboBufferInfo.range = sizeof(gBufferCameraProject);

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptors[0].descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &cameraUboBufferInfo;

        device->UpdateDescriptorSets(descriptorWrites.size(), descriptorWrites.data());
    }

    void GBufferPass::createPipelines() {
        pipelines.resize(1);
        VkPushConstantRange pushConstantRange = CreatePushConstantRange(VK_SHADER_STAGE_VERTEX_BIT,
                                                                        sizeof(PushConstBlock), 0);
        std::array<VkDescriptorSetLayout, 2> layouts = {descriptors[0].layout, descriptorSetLayoutImage};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        device->CreatePipelineLayout(&pipelineLayoutInfo, &pipelines[0].layout);

        auto vertShaderModule = device->CreateShaderModule(SCENE_GBUFFER_VERT);
        auto fragShaderModule = device->CreateShaderModule(SCENE_GBUFFER_FRAG);

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

        std::vector<VertexComponent> components{VertexComponent::Position, VertexComponent::UV, VertexComponent::Color,
                                                VertexComponent::Normal, VertexComponent::Tangent};

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
        rasterizer.cullMode = VK_CULL_MODE_NONE; //important
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

        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

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

    void GBufferPass::createFramebuffers() {
        framebuffer.attachments.resize(G_BUFFER_TYPE::g_count);
        createAttachment(
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                &framebuffer.attachments[G_BUFFER_TYPE::g_position]);

        // (World space) Normals
        createAttachment(
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                &framebuffer.attachments[G_BUFFER_TYPE::g_normal]);

        // Albedo (color)
        createAttachment(
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                &framebuffer.attachments[G_BUFFER_TYPE::g_albedo]);

        // Depth attachment

        // Find a suitable depth format
        VkFormat attDepthFormat = device->findDepthFormat();

        createAttachment(
                attDepthFormat,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                &framebuffer.attachments[g_depth]);
        std::array<VkImageView, 4> attachments;
        attachments[0] = framebuffer.attachments[G_BUFFER_TYPE::g_position].view;
        attachments[1] = framebuffer.attachments[G_BUFFER_TYPE::g_normal].view;
        attachments[2] = framebuffer.attachments[G_BUFFER_TYPE::g_albedo].view;
        attachments[3] = framebuffer.attachments[G_BUFFER_TYPE::g_depth].view;
        // Framebuffer
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = framebuffer.renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = device->getSwapchainInfo().extent.width;
        framebufferInfo.height = device->getSwapchainInfo().extent.height;
        framebufferInfo.layers = 1;
        device->CreateFramebuffer(&framebufferInfo, &framebuffer.framebuffer);
    }

    void GBufferPass::draw() {
        auto commandBuffer = device->getCurrentCommandBuffer();

        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1].layout,
                                0, 1, &descriptors[0].descriptorSet, 0, nullptr);

        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout,
                                0, 1, &descriptors[0].descriptorSet, 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].pipeline);

        PushConstBlock pushConstant;
        engineGlobalContext.getScene()->draw(device->getCurrentCommandBuffer(), RenderFlags::BindImages,
                                             pipelines[0].layout, 1, &pushConstant, sizeof(pushConstant));
    }

    void GBufferPass::createAttachment(VkFormat format, VkImageUsageFlagBits usage,
                                       FrameBufferAttachment *attachment) {
        VkImageAspectFlags aspectMask = 0;
        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
                aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        assert(aspectMask > 0);
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = device->getSwapchainInfo().extent.width;
        imageInfo.extent.height = device->getSwapchainInfo().extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.format = format;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        device->CreateImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachment->image, attachment->mem);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.format = format;
        viewInfo.subresourceRange = {};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.image = attachment->image;
        device->CreateImageView(&viewInfo, &attachment->view);
    }
}