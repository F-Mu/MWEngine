#include "pbr_ibl_pass.h"
#include "lut_pass.h"
#include "cube_pass.h"
#include "prefilter_env_map_frag.h"
#include "irradiance_cube_frag.h"
#include "deferred_vert.h"
#include "pbribl_frag.h"
#include "function/render/render_resource.h"

namespace MW {
    extern PassBase::Descriptor gBufferGlobalDescriptor;
    PassBase::Descriptor LightingGlobalDescriptor;

    void PbrIblPass::initialize(const RenderPassInitInfo *info) {
        PassBase::initialize(info);
        const auto *_info = static_cast<const PbrIblPassInitInfo *>(info);
        fatherFramebuffer = _info->frameBuffer;
        precompute(info);
        createUniformBuffer();
        createDescriptorSets();
        createPipelines();
        createLightingGlobalDescriptor();
    }

    void PbrIblPass::clean() {
        device->DestroyDescriptorSetLayout(LightingGlobalDescriptor.layout);
        for (auto &pipeline: pipelines) {
            device->DestroyPipeline(pipeline.pipeline);
            device->DestroyPipelineLayout(pipeline.layout);
        }
        for(auto&descriptor:descriptors){
            device->DestroyDescriptorSetLayout(descriptor.layout);
        }
        device->unMapMemory(pbrIblUboBuffer);
        device->DestroyVulkanBuffer(pbrIblUboBuffer);
        lutPass->clean();
        irradiancePass->clean();
        prefilteredPass->clean();
        lutPass.reset();
        irradiancePass.reset();
        prefilteredPass.reset();
        PassBase::clean();
    }

    void PbrIblPass::preparePassData() {
        pbrIblUbo.projViewMatrix = renderResource->cameraObject.projViewMatrix;
        pbrIblUbo.cameraPos = renderResource->cameraObject.position;
        memcpy(pbrIblUbo.lights, renderResource->cameraObject.lights, sizeof(pbrIblUbo.lights));
        memcpy(pbrIblUboBuffer.mapped, &pbrIblUbo, sizeof(pbrIblUbo));
    }

    void PbrIblPass::precompute(const RenderPassInitInfo *info) {
        prefilteredPass = std::make_shared<CubePass>();
        irradiancePass = std::make_shared<CubePass>();
        lutPass = std::make_shared<LutPass>();
        CubePassInitInfo prefilteredInfo(info), irradianceInfo(info);
        LutPassInitInfo lutInfo(info);
        prefilteredInfo.fragShader = &PREFILTER_ENV_MAP_FRAG;
        prefilteredInfo.type = prefilter_cube_pass;
        prefilteredInfo.pushBlock.pushBlock.data = new prefilterPushBlock();
        prefilteredInfo.pushBlock.size += sizeof(prefilterPushBlock);
        irradianceInfo.fragShader = &IRRADIANCE_CUBE_FRAG;
        irradianceInfo.pushBlock.pushBlock.data = new irradiancePushBlock();
        irradianceInfo.pushBlock.size += sizeof(irradiancePushBlock);
        irradianceInfo.type = irradiance_cube_pass;
        prefilteredPass->initialize(&prefilteredInfo);
        irradiancePass->initialize(&irradianceInfo);
        lutPass->initialize(&lutInfo);
        prefilteredPass->draw();
        irradiancePass->draw();
        lutPass->draw();
        delete static_cast<prefilterPushBlock *>(prefilteredInfo.pushBlock.pushBlock.data);
        delete static_cast<irradiancePushBlock *>(irradianceInfo.pushBlock.pushBlock.data);
    }

    void PbrIblPass::draw() {
        auto commandBuffer = device->getCurrentCommandBuffer();

        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout,
                                0, 1, &descriptors[0].descriptorSet, 0, nullptr);

        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout,
                                1, 1, &gBufferGlobalDescriptor.descriptorSet, 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].pipeline);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }

    void PbrIblPass::createUniformBuffer() {
        device->CreateBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                pbrIblUboBuffer,
                sizeof(pbrIblUbo));
        device->MapMemory(pbrIblUboBuffer);
    }

    void PbrIblPass::createDescriptorSets() {
        descriptors.resize(1);
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 1),
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 2),
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 3),
        };
        VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = CreateDescriptorSetLayoutCreateInfo(setLayoutBindings);

        device->CreateDescriptorSetLayout(&descriptorLayoutInfo, &descriptors[0].layout);
        device->CreateDescriptorSet(1, descriptors[0].layout, descriptors[0].descriptorSet);
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                         &pbrIblUboBuffer.descriptor),
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                         &irradiancePass->cubeMap.descriptor),
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
                                         &lutPass->lutTexture.descriptor),
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3,
                                         &prefilteredPass->cubeMap.descriptor),
        };
        device->UpdateDescriptorSets(static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data());

    }

    void PbrIblPass::createPipelines() {
        pipelines.resize(1);
        std::vector<VkDescriptorSetLayout> layouts = {descriptors[0].layout, gBufferGlobalDescriptor.layout};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        device->CreatePipelineLayout(&pipelineLayoutInfo, &pipelines[0].layout);

        auto vertShaderModule = device->CreateShaderModule(DEFERRED_VERT);
        auto fragShaderModule = device->CreateShaderModule(PBRIBL_FRAG);

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
        pipelineInfo.subpass = main_camera_subpass_lighting_pass;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        device->CreateGraphicsPipelines(&pipelineInfo, &pipelines[0].pipeline);

        device->DestroyShaderModule(fragShaderModule);
        device->DestroyShaderModule(vertShaderModule);
    }

    void PbrIblPass::createLightingGlobalDescriptor() {
        std::vector<VkDescriptorSetLayoutBinding> binding = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pBindings = binding.data();
        descriptorSetLayoutCreateInfo.bindingCount = binding.size();
        device->CreateDescriptorSetLayout(&descriptorSetLayoutCreateInfo, &LightingGlobalDescriptor.layout);
        device->CreateDescriptorSet(1, LightingGlobalDescriptor.layout, LightingGlobalDescriptor.descriptorSet);
        VkDescriptorImageInfo imageInfos{};
        imageInfos.sampler = VK_NULL_HANDLE; //why NULL_HANDLE
        imageInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos.imageView = fatherFramebuffer->attachments[main_camera_lighting].view;
        VkWriteDescriptorSet descriptorWrites{};

        descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites.dstSet = LightingGlobalDescriptor.descriptorSet;
        descriptorWrites.dstBinding = 0;
        descriptorWrites.dstArrayElement = 0;
        descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites.descriptorCount = 1;
        descriptorWrites.pImageInfo = &imageInfos;

        device->UpdateDescriptorSets(1, &descriptorWrites);
    }

    void PbrIblPass::updateAfterFramebufferRecreate() {
        createDescriptorSets();
        createLightingGlobalDescriptor();
    }
}