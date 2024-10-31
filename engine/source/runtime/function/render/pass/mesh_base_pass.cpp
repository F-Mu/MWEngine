#include "mesh_base_pass.h"
#include "function/global/engine_global_context.h"
#include "function/render/render_system.h"
#include "function/render/render_camera.h"
#include "function/render/render_resource.h"
#include "function/render/render_model.h"
#include "function/global/engine_global_context.h"
#include "function/render/scene_manager.h"
#include "base_pass_ms_mesh.h"
#include "scene_gbuffer_frag.h"
#include "depthpass_vert.h"
#if USE_MESH_SHADER
namespace MW {
    void MeshBaseBufferPass::createDescriptorSets() {
        descriptors.resize(1);
        std::vector<VkDescriptorSetLayoutBinding> binding = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_MESH_BIT_NV, 0),
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
    void MeshBaseBufferPass::createPipelines() {
        pipelines.resize(1);
        VkPushConstantRange pushConstantRange = CreatePushConstantRange(VK_SHADER_STAGE_MESH_BIT_NV,
                                                                        sizeof(PushConstBlock), 0);
        std::vector<VkDescriptorSetLayout> layouts = {descriptors[0].layout, descriptorSetLayoutImage,
                                                      descriptorSetLayoutVertexStorage,
                                                      descriptorSetLayoutMeshlet};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        device->CreatePipelineLayout(&pipelineLayoutInfo, &pipelines[0].layout);

        auto meshShaderModule = device->CreateShaderModule(BASE_PASS_MS_MESH);
        auto fragShaderModule = device->CreateShaderModule(SCENE_GBUFFER_FRAG);

        VkPipelineShaderStageCreateInfo meshShaderStageInfo{};
        meshShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        meshShaderStageInfo.stage = VK_SHADER_STAGE_MESH_BIT_NV;
        meshShaderStageInfo.module = meshShaderModule;
        meshShaderStageInfo.pName = "main";

        auto camera = engineGlobalContext.renderSystem->getRenderCamera();
        struct SpecializationData {
            float znear;
            float zfar;
        } specializationData;
        specializationData.znear = camera->getNearClip();
        specializationData.zfar = camera->getFarClip();
        std::array<VkSpecializationMapEntry, 2> specializationMapEntries = {
                CreateSpecializationMapEntry(0, offsetof(SpecializationData, znear),
                                             sizeof(SpecializationData::znear)),
                CreateSpecializationMapEntry(1, offsetof(SpecializationData, zfar),
                                             sizeof(SpecializationData::zfar))
        };
        auto specializationInfo = CreateSpecializationInfo(2, specializationMapEntries.data(),
                                                           sizeof(specializationData), &specializationData);
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        fragShaderStageInfo.pSpecializationInfo = &specializationInfo;

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {meshShaderStageInfo, fragShaderStageInfo};

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
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates = {
                CreatePipelineColorBlendAttachmentState(0xf, VK_FALSE),
                CreatePipelineColorBlendAttachmentState(0xf, VK_FALSE),
                CreatePipelineColorBlendAttachmentState(0xf, VK_FALSE),
                CreatePipelineColorBlendAttachmentState(0xf, VK_FALSE)
        };

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = blendAttachmentStates.size();
        colorBlending.pAttachments = blendAttachmentStates.data();
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
        depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
        depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
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
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = nullptr;
        pipelineInfo.pInputAssemblyState = nullptr;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.pDepthStencilState = &depthStencilCreateInfo;
        pipelineInfo.layout = pipelines[0].layout;
        pipelineInfo.renderPass = fatherFramebuffer->renderPass;
        pipelineInfo.subpass = main_camera_subpass_g_buffer_pass;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        device->CreateGraphicsPipelines(&pipelineInfo, &pipelines[0].pipeline);

        device->DestroyShaderModule(fragShaderModule);
        device->DestroyShaderModule(meshShaderModule);
    }

    void MeshBaseBufferPass::draw() {
        auto commandBuffer = device->getCurrentCommandBuffer();

        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout,
                                0, 1, &descriptors[0].descriptorSet, 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].pipeline);

        PushConstBlock pushConstant;
        engineGlobalContext.getScene()->draw(device->getCurrentCommandBuffer(), RenderFlags::BindImages,
                                             pipelines[0].layout, 1, &pushConstant, sizeof(pushConstant), true);
    }
}
#endif