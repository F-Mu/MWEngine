#include "ssao_pass.h"
#include "function/render/render_model.h"
#include "function/render/render_resource.h"
#include "deferred_vert.h"
#include "ssao_frag.h"
#include <random>

namespace MW {
    extern PassBase::Descriptor gBufferGlobalDescriptor;
    PassBase::Descriptor SSAOGlobalDescriptor;

    constexpr float lerp(float a, float b, float f) {
        return a + f * (b - a);
    }

    void SSAOPass::initialize(const RenderPassInitInfo *info) {
        PassBase::initialize(info);

        const auto *_info = static_cast<const SSAOPassInitInfo *>(info);
        fatherFramebuffer = _info->frameBuffer;
        createUniformBuffer();
        createDescriptorSets();
        createPipelines();
        createSSAOGlobalDescriptor();
    }

    void SSAOPass::clean() {
        device->DestroyDescriptorSetLayout(SSAOGlobalDescriptor.layout);
        for (auto &pipeline: pipelines) {
            device->DestroyPipeline(pipeline.pipeline);
            device->DestroyPipelineLayout(pipeline.layout);
        }
        for (auto &descriptor: descriptors) {
            device->DestroyDescriptorSetLayout(descriptor.layout);
        }
        device->unMapMemory(cameraUboBuffer);
        device->DestroyVulkanBuffer(cameraUboBuffer);
        device->unMapMemory(SSAOKernelBuffer);
        device->DestroyVulkanBuffer(SSAOKernelBuffer);
        SSAONoise.destroy(device);
        PassBase::clean();
    }

    void SSAOPass::preparePassData() {
        SSAOUbo.projection = renderResource->cameraObject.projMatrix;
        memcpy(cameraUboBuffer.mapped, &SSAOUbo, sizeof(SSAOUbo));
    }

    void SSAOPass::createUniformBuffer() {
        device->CreateBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                cameraUboBuffer,
                sizeof(SSAOUbo));
        device->MapMemory(cameraUboBuffer);
        // SSAO
        std::default_random_engine rndEngine((unsigned) time(nullptr));
        std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

        // Sample kernel
        std::vector<glm::vec4> ssaoKernel(SSAO_KERNEL_SIZE);
        for (uint32_t i = 0; i < SSAO_KERNEL_SIZE; ++i) {
            glm::vec3 sample(rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine));
            sample = glm::normalize(sample);
            sample *= rndDist(rndEngine);
            float scale = float(i) / float(SSAO_KERNEL_SIZE);
            scale = lerp(0.1f, 1.0f, scale * scale);
            ssaoKernel[i] = glm::vec4(sample * scale, 0.0f);
        }
        // Upload as UBO
        device->CreateBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                SSAOKernelBuffer,
                ssaoKernel.size() * sizeof(glm::vec4),
                ssaoKernel.data());

        // Random noise
        std::vector<glm::vec4> ssaoNoise(SSAO_NOISE_DIM * SSAO_NOISE_DIM);
        for (uint32_t i = 0; i < static_cast<uint32_t>(ssaoNoise.size()); i++)
            ssaoNoise[i] = glm::vec4(rndDist(rndEngine) * 2.0f - 1.0f, rndDist(rndEngine) * 2.0f - 1.0f, 0.0f, 0.0f);
        // Upload as texture
        SSAONoise.fromBuffer(ssaoNoise.data(), ssaoNoise.size() * sizeof(glm::vec4), VK_FORMAT_R32G32B32A32_SFLOAT,
                             SSAO_NOISE_DIM, SSAO_NOISE_DIM, device, VK_FILTER_NEAREST);
    }

    void SSAOPass::createDescriptorSets() {
        descriptors.resize(1);
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT,
                                                 0),                        // FS Position+Depth
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 1),                        // FS Normals
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 2),
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4)
        };
        auto setLayoutCreateInfo = CreateDescriptorSetLayoutCreateInfo(setLayoutBindings.data(),
                                                                       static_cast<uint32_t>(setLayoutBindings.size()));
        device->CreateDescriptorSetLayout(&setLayoutCreateInfo, &descriptors[0].layout);
        device->CreateDescriptorSet(1, descriptors[0].layout, descriptors[0].descriptorSet);
        std::vector<VkDescriptorImageInfo> imageDescriptors = {
                CreateDescriptorImageInfo(device->getOrCreateDefaultSampler(VK_FILTER_NEAREST),
                                          fatherFramebuffer->attachments[g_buffer_view_position].view,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                CreateDescriptorImageInfo(device->getOrCreateDefaultSampler(VK_FILTER_NEAREST),
                                          fatherFramebuffer->attachments[g_buffer_normal].view,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        };
        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0,
                                         &imageDescriptors[0]),                    // FS Position+Depth
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                         &imageDescriptors[1]),                    // FS Normals
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2,
                                         &SSAONoise.descriptor),        // FS SSAO Noise
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3,
                                         &SSAOKernelBuffer.descriptor),        // FS SSAO Kernel UBO
                CreateWriteDescriptorSet(descriptors[0].descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4,
                                         &cameraUboBuffer.descriptor),        // FS SSAO Params UBO
        };
        device->UpdateDescriptorSets(static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data());
    }

    void SSAOPass::createPipelines() {
        pipelines.resize(1);
        std::vector<VkDescriptorSetLayout> layouts = {descriptors[0].layout};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        device->CreatePipelineLayout(&pipelineLayoutInfo, &pipelines[0].layout);

        auto vertShaderModule = device->CreateShaderModule(DEFERRED_VERT);
        auto fragShaderModule = device->CreateShaderModule(SSAO_FRAG);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        struct SpecializationData {
            uint32_t kernelSize = SSAO_KERNEL_SIZE;
            float radius = SSAO_RADIUS;
        } specializationData;
        std::array<VkSpecializationMapEntry, 2> specializationMapEntries = {
                CreateSpecializationMapEntry(0, offsetof(SpecializationData, kernelSize),
                                             sizeof(SpecializationData::kernelSize)),
                CreateSpecializationMapEntry(1, offsetof(SpecializationData, radius),
                                             sizeof(SpecializationData::radius))
        };
        auto specializationInfo = CreateSpecializationInfo(2, specializationMapEntries.data(),
                                                           sizeof(specializationData), &specializationData);
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
        fragShaderStageInfo.pSpecializationInfo = &specializationInfo;
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
        pipelineInfo.subpass = main_camera_subpass_ssao_pass;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        device->CreateGraphicsPipelines(&pipelineInfo, &pipelines[0].pipeline);

        device->DestroyShaderModule(fragShaderModule);
        device->DestroyShaderModule(vertShaderModule);
    }

    void SSAOPass::draw() {
        auto commandBuffer = device->getCurrentCommandBuffer();
        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout,
                                0, 1, &descriptors[0].descriptorSet, 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].pipeline);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }

    void SSAOPass::createSSAOGlobalDescriptor() {
        std::vector<VkDescriptorSetLayoutBinding> binding = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pBindings = binding.data();
        descriptorSetLayoutCreateInfo.bindingCount = binding.size();
        device->CreateDescriptorSetLayout(&descriptorSetLayoutCreateInfo, &SSAOGlobalDescriptor.layout);
        device->CreateDescriptorSet(1, SSAOGlobalDescriptor.layout, SSAOGlobalDescriptor.descriptorSet);
        VkDescriptorImageInfo imageInfos{};
        imageInfos.sampler = VK_NULL_HANDLE; //why NULL_HANDLE
        imageInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos.imageView = fatherFramebuffer->attachments[main_camera_ao].view;
        VkWriteDescriptorSet descriptorWrites{};

        descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites.dstSet = SSAOGlobalDescriptor.descriptorSet;
        descriptorWrites.dstBinding = 0;
        descriptorWrites.dstArrayElement = 0;
        descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites.descriptorCount = 1;
        descriptorWrites.pImageInfo = &imageInfos;

        device->UpdateDescriptorSets(1, &descriptorWrites);
    }

    void SSAOPass::updateAfterFramebufferRecreate() {
        createDescriptorSets();
        createSSAOGlobalDescriptor();
    }

}