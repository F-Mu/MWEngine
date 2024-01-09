#include <map>
#include <stdexcept>
#include <array>
#include <iostream>
#include "main_camera_pass.h"
#include "function/render/render_resource.h"
#include "cascade_shadow_map_pass.h"
#include "g_buffer_pass.h"
#include "deferred_csm_pass.h"
#include "ssao_pass.h"
#include "function/global/engine_global_context.h"
#include "function/render/scene_manager.h"
#include "shading_pass.h"
#include "scene_frag.h"
#include "scene_vert.h"

namespace MW {
    extern VkDescriptorSetLayout descriptorSetLayoutImage;

    void MainCameraPass::initialize(const RenderPassInitInfo *init_info) {
        PassBase::initialize(init_info);
        createAttachments();
        createRenderPass();
        createUniformBuffer();
        createDescriptorSets();
//        createPipelines();
        createSwapchainFramebuffers();
        /* 注意顺序,gBuffer中有global的descriptorSet */
        gBufferPass = std::make_shared<GBufferPass>();
        GBufferPassInitInfo gBufferInfo(init_info);
        gBufferInfo.frameBuffer = &framebuffer;
        gBufferPass->initialize(&gBufferInfo);

        shadowMapPass = std::make_shared<DeferredCSMPass>();
        CSMPassInitInfo csmInfo(init_info);
        csmInfo.frameBuffer = &framebuffer;
        shadowMapPass->initialize(&csmInfo);

        ssaoPass = std::make_shared<SSAOPass>();
        SSAOPassInitInfo ssaoInfo(init_info);
        ssaoInfo.frameBuffer = &framebuffer;
        ssaoPass->initialize(&ssaoInfo);

        shadingPass = std::make_shared<ShadingPass>();
        ShadingPassInitInfo shadingInfo(init_info);
        shadingInfo.frameBuffer = &framebuffer;
        shadingPass->initialize(&shadingInfo);
    }

    void MainCameraPass::createRenderPass() {
        std::array<VkAttachmentDescription, main_camera_type_count> attachments{};

        /* 假设所有attachment的部分格式是一样的 */
        for (auto &attachmentDesc: attachments) {
            attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        for (int i = 0; i < main_camera_g_buffer_type_count + main_camera_custom_type_count; ++i) {
            attachments[i].format = framebuffer.attachments[i].format;
            attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        VkAttachmentDescription &depthAttachment = attachments[main_camera_depth];
        depthAttachment.format = device->getDepthImageInfo().depthImageFormat;

        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription &colorAttachment = attachments[main_camera_swap_chain_image];
        colorAttachment.format = device->getSwapchainInfo().imageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

//        VkAttachmentReference depthAttachmentRef{};
//        depthAttachmentRef.attachment = main_camera_depth;
//        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

//        VkAttachmentReference colorAttachmentRef = {};
//        colorAttachmentRef.attachment = main_camera_swap_chain_image;
//        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Create G-Buffer SubPass;
        std::array<VkSubpassDescription, main_camera_subpass_count> subpassDescs{};
        std::array<VkAttachmentReference, 4> gBufferAttachmentReferences{};
        gBufferAttachmentReferences[g_buffer_position].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        gBufferAttachmentReferences[g_buffer_position].attachment = g_buffer_position;
        gBufferAttachmentReferences[g_buffer_normal].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        gBufferAttachmentReferences[g_buffer_normal].attachment = g_buffer_normal;
        gBufferAttachmentReferences[g_buffer_albedo].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        gBufferAttachmentReferences[g_buffer_albedo].attachment = g_buffer_albedo;
        gBufferAttachmentReferences[g_buffer_view_position].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        gBufferAttachmentReferences[g_buffer_view_position].attachment = g_buffer_view_position;

        VkAttachmentReference depthAttachmentReference{};
        depthAttachmentReference.attachment = main_camera_depth;
        depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpassDescs[main_camera_subpass_g_buffer_pass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescs[main_camera_subpass_g_buffer_pass].colorAttachmentCount = gBufferAttachmentReferences.size();
        subpassDescs[main_camera_subpass_g_buffer_pass].pColorAttachments = gBufferAttachmentReferences.data();
        subpassDescs[main_camera_subpass_g_buffer_pass].pDepthStencilAttachment = &depthAttachmentReference;
        std::array<VkAttachmentReference, 4> ScreenSpaceInputAttachmentReferences{};
        ScreenSpaceInputAttachmentReferences[g_buffer_position].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ScreenSpaceInputAttachmentReferences[g_buffer_position].attachment = g_buffer_position;
        ScreenSpaceInputAttachmentReferences[g_buffer_normal].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ScreenSpaceInputAttachmentReferences[g_buffer_normal].attachment = g_buffer_normal;
        ScreenSpaceInputAttachmentReferences[g_buffer_albedo].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ScreenSpaceInputAttachmentReferences[g_buffer_albedo].attachment = g_buffer_albedo;
        ScreenSpaceInputAttachmentReferences[g_buffer_view_position].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ScreenSpaceInputAttachmentReferences[g_buffer_view_position].attachment = g_buffer_view_position;

        // Create CSM SubPass;
        std::array<VkAttachmentReference, 1> CSMOutputAttachmentReferences{};
        CSMOutputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        CSMOutputAttachmentReferences[0].attachment = main_camera_shadow;

        subpassDescs[main_camera_subpass_csm_pass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescs[main_camera_subpass_csm_pass].inputAttachmentCount = ScreenSpaceInputAttachmentReferences.size();
        subpassDescs[main_camera_subpass_csm_pass].pInputAttachments = ScreenSpaceInputAttachmentReferences.data();
        subpassDescs[main_camera_subpass_csm_pass].colorAttachmentCount = CSMOutputAttachmentReferences.size();
        subpassDescs[main_camera_subpass_csm_pass].pColorAttachments = CSMOutputAttachmentReferences.data();
        subpassDescs[main_camera_subpass_csm_pass].pDepthStencilAttachment = &depthAttachmentReference;

        // Create SSAO SubPass;
        std::array<VkAttachmentReference, 2> SSAOInputAttachmentReferences{};
        SSAOInputAttachmentReferences[g_buffer_position].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SSAOInputAttachmentReferences[g_buffer_position].attachment = g_buffer_position;
        SSAOInputAttachmentReferences[g_buffer_normal].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SSAOInputAttachmentReferences[g_buffer_normal].attachment = g_buffer_normal;

        std::array<VkAttachmentReference, 1> SSAOOutputAttachmentReferences{};
        SSAOOutputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        SSAOOutputAttachmentReferences[0].attachment = main_camera_ao;

        subpassDescs[main_camera_subpass_ssao_pass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescs[main_camera_subpass_ssao_pass].inputAttachmentCount = SSAOInputAttachmentReferences.size();
        subpassDescs[main_camera_subpass_ssao_pass].pInputAttachments = SSAOInputAttachmentReferences.data();
        subpassDescs[main_camera_subpass_ssao_pass].colorAttachmentCount = SSAOOutputAttachmentReferences.size();
        subpassDescs[main_camera_subpass_ssao_pass].pColorAttachments = SSAOOutputAttachmentReferences.data();
        subpassDescs[main_camera_subpass_ssao_pass].pDepthStencilAttachment = &depthAttachmentReference;

        // Create Shading SubPass;
        std::array<VkAttachmentReference, 2> ShadingInputAttachmentReferences{};
        ShadingInputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ShadingInputAttachmentReferences[0].attachment = main_camera_shadow;
        ShadingInputAttachmentReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        ShadingInputAttachmentReferences[1].attachment = main_camera_ao;

        std::array<VkAttachmentReference, 1> ShadingOutputAttachmentReferences{};
        ShadingOutputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ShadingOutputAttachmentReferences[0].attachment = main_camera_swap_chain_image;
        subpassDescs[main_camera_subpass_shading_pass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescs[main_camera_subpass_shading_pass].inputAttachmentCount = ShadingInputAttachmentReferences.size();
        subpassDescs[main_camera_subpass_shading_pass].pInputAttachments = ShadingInputAttachmentReferences.data();
        subpassDescs[main_camera_subpass_shading_pass].colorAttachmentCount = ShadingOutputAttachmentReferences.size();
        subpassDescs[main_camera_subpass_shading_pass].pColorAttachments = ShadingOutputAttachmentReferences.data();
        subpassDescs[main_camera_subpass_shading_pass].pDepthStencilAttachment = &depthAttachmentReference;

        std::array<VkSubpassDependency, 6> dependencies = {};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = main_camera_subpass_g_buffer_pass;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dependencyFlags = 0;

        dependencies[1].srcSubpass = main_camera_subpass_g_buffer_pass;
        dependencies[1].dstSubpass = main_camera_subpass_csm_pass;
        dependencies[1].srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask =
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[2].srcSubpass = main_camera_subpass_csm_pass;
        dependencies[2].dstSubpass = main_camera_subpass_shading_pass;
        dependencies[2].srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[2].dstStageMask =
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[3].srcSubpass = main_camera_subpass_g_buffer_pass;
        dependencies[3].dstSubpass = main_camera_subpass_ssao_pass;
        dependencies[3].srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[3].dstStageMask =
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[3].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[4].srcSubpass = main_camera_subpass_ssao_pass;
        dependencies[4].dstSubpass = main_camera_subpass_shading_pass;
        dependencies[4].srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[4].dstStageMask =
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[4].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[5].srcSubpass = main_camera_subpass_csm_pass;
        dependencies[5].dstSubpass = main_camera_subpass_ssao_pass;
        dependencies[5].srcStageMask =
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dependencies[5].dstStageMask =
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dependencies[5].srcAccessMask = VK_ACCESS_NONE_KHR;
        dependencies[5].dstAccessMask = VK_ACCESS_NONE_KHR;
        dependencies[5].dependencyFlags = 0;
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = subpassDescs.size();
        renderPassInfo.pSubpasses = subpassDescs.data();
        renderPassInfo.dependencyCount = dependencies.size();
        renderPassInfo.pDependencies = dependencies.data();

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
            std::vector<VkImageView> attachments = {
                    framebuffer.attachments[g_buffer_position].view,
                    framebuffer.attachments[g_buffer_normal].view,
                    framebuffer.attachments[g_buffer_albedo].view,
                    framebuffer.attachments[g_buffer_view_position].view,
                    framebuffer.attachments[main_camera_shadow].view,
                    framebuffer.attachments[main_camera_ao].view,
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

            device->CreateFramebuffer(&framebufferInfo, &swapChainFramebuffers[i]);
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

        std::array<VkClearValue, main_camera_g_buffer_type_count + main_camera_custom_type_count + 2> clearColors{};
        for (int i = 0; i < main_camera_g_buffer_type_count + main_camera_custom_type_count; ++i)
            clearColors[i] = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearColors[main_camera_depth] = {1.0f, 0};
        clearColors[main_camera_swap_chain_image] = {{0.0f, 0.0f, 0.2f, 1.0f}};
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

        gBufferPass->draw();

        vkCmdNextSubpass(device->getCurrentCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);

        shadowMapPass->draw();
        vkCmdNextSubpass(device->getCurrentCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);

        ssaoPass->draw();
        vkCmdNextSubpass(device->getCurrentCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);

        shadingPass->draw();
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
        createAttachments();
        createSwapchainFramebuffers();
        gBufferPass->updateAfterFramebufferRecreate();
    }

    void MainCameraPass::updateCamera() {
        memcpy(cameraUniformBuffer.mapped, &renderResource->cameraObject, sizeof(renderResource->cameraObject));
    }

    void MainCameraPass::preparePassData() {
        updateCamera();
        gBufferPass->preparePassData();
        shadowMapPass->preparePassData();
        ssaoPass->preparePassData();
    }

    void MainCameraPass::loadModel() {
        const uint32_t glTFLoadingFlags =
//                FileLoadingFlags::PreTransformVertices | FileLoadingFlags::PreMultiplyVertexColors |
                FileLoadingFlags::PreTransformVertices |
                FileLoadingFlags::FlipY;
        scene.loadFromFile(getAssetPath() + "models/sponza/sponza.gltf", device.get(), glTFLoadingFlags);
    }

    void MainCameraPass::createAttachment(VkFormat format, VkImageUsageFlagBits usage,
                                          FrameBufferAttachment *attachment) {
        VkImageAspectFlags aspectMask = 0;
        attachment->format = format;
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
        imageInfo.usage = usage | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        device->CreateImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachment->image, attachment->mem);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange = {};
        viewInfo.subresourceRange.aspectMask = aspectMask;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.image = attachment->image;
        device->CreateImageView(&viewInfo, &attachment->view);
    }

    void MainCameraPass::createAttachments() {
        framebuffer.attachments.resize(main_camera_g_buffer_type_count + main_camera_custom_type_count);
        // World space position
        createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[g_buffer_position]);

        // (World space) Normals
        createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[g_buffer_normal]);

        // Albedo (color)
        createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[g_buffer_albedo]);

        // Camera space position
        createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[g_buffer_view_position]);

        // shadow (color)
        createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[main_camera_shadow]);

        // ssao (color)
        createAttachment(VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[main_camera_ao]);
    }
}