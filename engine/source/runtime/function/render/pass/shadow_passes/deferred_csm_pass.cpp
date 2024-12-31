#include "deferred_csm_pass.h"
#include "function/global/engine_global_context.h"
#include "function/render/scene_manager.h"
#include "deferred_vert.h"
#include "deferred_csm_frag.h"

namespace MW {
    extern PassBase::Descriptor gBufferGlobalDescriptor;

    void DeferredCSMPass::initialize(const RenderPassInitInfo *info) {
        PassBase::initialize(info);

        const auto *_info = static_cast<const CSMPassInitInfo *>(info);
        fatherFramebuffer = _info->frameBuffer;
        DepthPassInitInfo depthInfo{info};
        depthInfo.depthArrayLayers = CASCADE_COUNT;
        depthInfo.depthImageWidth = DEFAULT_IMAGE_WIDTH;
        depthInfo.depthImageWidth = DEFAULT_IMAGE_HEIGHT;
        depthPass = std::make_shared<DepthPass>();
        depthPass->initialize(&depthInfo);

        skybox = engineGlobalContext.getScene()->getSkyBox();
//        createRenderPass();

        createUniformBuffer();
        createDescriptorSets();
        createPipelines();
        createCSMGlobalDescriptor();
    }

//    void DeferredCSMPass::clean() {
//        device->DestroyDescriptorSetLayout(CSMGlobalDescriptor.layout);
//        for (auto &pipeline: pipelines) {
//            device->DestroyPipeline(pipeline.pipeline);
//            device->DestroyPipelineLayout(pipeline.layout);
//        }
//        for(auto&descriptor:descriptors){
//            device->DestroyDescriptorSetLayout(descriptor.layout);
//        }
//        device->unMapMemory(shadowMapFSBuffer);
//        device->DestroyVulkanBuffer(shadowMapFSBuffer);
//        device->unMapMemory(cameraUboBuffer);
//        device->DestroyVulkanBuffer(cameraUboBuffer);
//        skybox->destroy(device);
//        depthPass->clean();
//
//        PassBase::clean();
//    }

    void DeferredCSMPass::createDescriptorSets() {
        descriptors.resize(1);
        std::vector<VkDescriptorSetLayoutBinding> binding = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 1),
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 3)
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pBindings = binding.data();
        descriptorSetLayoutCreateInfo.bindingCount = binding.size();
        device->CreateDescriptorSetLayout(&descriptorSetLayoutCreateInfo, &descriptors[0].layout);
        device->CreateDescriptorSet(1, descriptors[0].layout, descriptors[0].descriptorSet);
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                         &cameraUboBuffer.descriptor),
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                         &depthPass->depth.descriptor),
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2,
                                         &shadowMapFSBuffer.descriptor),
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3,
                                         &skybox->descriptor)
        };
        device->UpdateDescriptorSets(writeDescriptorSets.size(), writeDescriptorSets.data());
    }

    void DeferredCSMPass::createPipelines() {
        pipelines.resize(1);
        std::vector<VkDescriptorSetLayout> layouts = {descriptors[0].layout, gBufferGlobalDescriptor.layout};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        device->CreatePipelineLayout(&pipelineLayoutInfo, &pipelines[0].layout);

        auto vertShaderModule = device->CreateShaderModule(DEFERRED_VERT);
        auto fragShaderModule = device->CreateShaderModule(DEFERRED_CSM_FRAG);

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

        std::vector<VertexComponent> components{};

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
        depthStencilCreateInfo.depthTestEnable = VK_FALSE;
        depthStencilCreateInfo.depthWriteEnable = VK_FALSE;
        depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
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
        pipelineInfo.renderPass = fatherFramebuffer->renderPass;
        pipelineInfo.subpass = main_camera_subpass_csm_pass;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        device->CreateGraphicsPipelines(&pipelineInfo, &pipelines[0].pipeline);

        device->DestroyShaderModule(fragShaderModule);
        device->DestroyShaderModule(vertShaderModule);
    }

    void DeferredCSMPass::draw() {
        auto commandBuffer = device->getCurrentCommandBuffer();

        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout,
                                0, 1, &descriptors[0].descriptorSet, 0, nullptr);

        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout,
                                1, 1, &gBufferGlobalDescriptor.descriptorSet, 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].pipeline);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }


}