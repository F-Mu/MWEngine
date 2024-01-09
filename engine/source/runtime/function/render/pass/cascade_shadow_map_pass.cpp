#include "cascade_shadow_map_pass.h"
#include "function/global/engine_global_context.h"
#include "function/render/render_system.h"
#include "function/render/render_camera.h"
#include "function/render/render_resource.h"
#include "function/render/render_model.h"
#include "function/global/engine_global_context.h"
#include "function/render/scene_manager.h"
#include "cascade_shadow_map_vert.h"
#include "cascade_shadow_map_frag.h"
#include "debugshadowmap_vert.h"
#include "debugshadowmap_frag.h"
#include "depthpass_vert.h"

namespace MW {
    PassBase::Descriptor CSMGlobalDescriptor;

    void CascadeShadowMapPass::initialize(const RenderPassInitInfo *info) {
        PassBase::initialize(info);

        const auto *_info = static_cast<const CSMPassInitInfo *>(info);
        fatherFramebuffer = _info->frameBuffer;
        DepthPassInitInfo depthInfo{info};
        depthInfo.depthArrayLayers = CASCADE_COUNT;
        depthInfo.depthImageWidth = DEFAULT_IMAGE_WIDTH;
        depthInfo.depthImageWidth = DEFAULT_IMAGE_HEIGHT;
        depthPass = std::make_shared<DepthPass>();
        depthPass->initialize(&depthInfo);

//        createRenderPass();
        createUniformBuffer();
        createDescriptorSets();
        createPipelines();
        createCSMGlobalDescriptor();
    }

    void CascadeShadowMapPass::preparePassData() {
        /* 原神分享会：8级cascade，前4级每帧更新，后4级8帧之内更新 */
        static uint32_t frame = -1;
        frame = (frame + 1) % CASCADE_COUNT;
        depthPass->needUpdate.clear();
        for (int i = 0; i < MIN_CASCADE_COUNT; ++i)
            depthPass->needUpdate.emplace_back(i);
        for (int i = MIN_CASCADE_COUNT; i < CASCADE_COUNT; ++i)
            if (frame % (2 * (i - MIN_CASCADE_COUNT + 1)) == 0)
                depthPass->needUpdate.emplace_back(i);
        {
            static float lastSplitLambda = -1;
            if (lastSplitLambda != cascadeSplitLambda) {
                setCascadeSplits();
                lastSplitLambda = cascadeSplitLambda;
            }
        }
        updateCascade();

        for (int i = 0; i < CASCADE_COUNT; ++i)
            depthPass->uniformBufferObjects[i].projViewMatrix = cascades[i].lightProjViewMat;

        csmCameraProject.projection = renderResource->cameraObject.projMatrix;
        csmCameraProject.view = renderResource->cameraObject.viewMatrix;
        memcpy(cameraUboBuffer.mapped, &csmCameraProject, sizeof(csmCameraProject));
        for (uint32_t i = 0; i < CASCADE_COUNT; i++) {
            shadowMapFSUbo.cascadeSplits[i] = cascades[i].splitDepth;
            shadowMapFSUbo.cascadeProjViewMat[i] = cascades[i].lightProjViewMat;
        }
        shadowMapFSUbo.inverseViewMat = glm::inverse(renderResource->cameraObject.viewMatrix);
        shadowMapFSUbo.lightDir = normalize(-renderResource->cameraObject.lightPos);
//        shadowMapFSUbo.colorCascades = colorCascades;
        memcpy(shadowMapFSBuffer.mapped, &shadowMapFSUbo, sizeof(shadowMapFSUbo));
        depthPass->preparePassData();
    }

//    void CascadeShadowMapPass::createRenderPass() {
//        VkAttachmentDescription attachmentDescription{};
//        attachmentDescription.format = device->getDepthImageInfo().depthImageFormat;
//        attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
//        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//        attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
//
//        VkAttachmentReference depthReference = {};
//        depthReference.attachment = 0;
//        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//        VkSubpassDescription subpass = {};
//        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//        subpass.colorAttachmentCount = 0;
//        subpass.pDepthStencilAttachment = &depthReference;
//
//        std::array<VkSubpassDependency, 2> dependencies;
//
//        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
//        dependencies[0].dstSubpass = 0;
//        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
//        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
//
//        dependencies[1].srcSubpass = 0;
//        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
//        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
//        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//        dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
//
//        VkRenderPassCreateInfo renderPassCreateInfo = {};
//        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//        renderPassCreateInfo.attachmentCount = 1;
//        renderPassCreateInfo.pAttachments = &attachmentDescription;
//        renderPassCreateInfo.subpassCount = 1;
//        renderPassCreateInfo.pSubpasses = &subpass;
//        renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
//        renderPassCreateInfo.pDependencies = dependencies.data();
//
//        device->CreateRenderPass(&renderPassCreateInfo, &framebuffer.renderPass);
//    }

    void CascadeShadowMapPass::createUniformBuffer() {
        device->CreateBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                cameraUboBuffer,
                sizeof(csmCameraProject));
        device->MapMemory(cameraUboBuffer);
        device->CreateBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                shadowMapFSBuffer,
                sizeof(shadowMapFSUbo));
        device->MapMemory(shadowMapFSBuffer);
    }

    void CascadeShadowMapPass::createDescriptorSets() {
        descriptors.resize(1);
        std::vector<VkDescriptorSetLayoutBinding> binding = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 VK_SHADER_STAGE_FRAGMENT_BIT, 1),
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
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
        cameraUboBufferInfo.range = sizeof(csmCameraProject);

        VkDescriptorImageInfo shadowMapImageInfo{};
        shadowMapImageInfo.sampler = depthPass->depth.sampler;
        shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        shadowMapImageInfo.imageView = depthPass->depth.view;

        VkDescriptorBufferInfo shadowMapFSBufferInfo{};
        shadowMapFSBufferInfo.buffer = shadowMapFSBuffer.buffer;
        shadowMapFSBufferInfo.offset = 0;
        shadowMapFSBufferInfo.range = sizeof(shadowMapFSUbo);

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptors[0].descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &cameraUboBufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptors[0].descriptorSet;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &shadowMapImageInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptors[0].descriptorSet;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &shadowMapFSBufferInfo;

        device->UpdateDescriptorSets(descriptorWrites.size(), descriptorWrites.data());
    }

    void CascadeShadowMapPass::createPipelines() {
        pipelines.resize(2);
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

        auto vertShaderModule = device->CreateShaderModule(CASCADE_SHADOW_MAP_VERT);
        auto fragShaderModule = device->CreateShaderModule(CASCADE_SHADOW_MAP_FRAG);

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
                                                VertexComponent::Normal};

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
        pipelineInfo.renderPass = fatherFramebuffer->renderPass;
        pipelineInfo.subpass = main_camera_subpass_csm_pass;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        device->CreateGraphicsPipelines(&pipelineInfo, &pipelines[0].pipeline);

        device->DestroyShaderModule(fragShaderModule);
        device->DestroyShaderModule(vertShaderModule);

        //debug pipeline
        {
            pushConstantRange = CreatePushConstantRange(VK_SHADER_STAGE_VERTEX_BIT,
                                                        sizeof(debugPushConstant), 0);
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
            device->CreatePipelineLayout(&pipelineLayoutInfo, &pipelines[1].layout);

            VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            pipelineInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
            pipelineInfo.layout = pipelines[1].layout;
            vertShaderModule = device->CreateShaderModule(DEBUGSHADOWMAP_VERT);
            fragShaderModule = device->CreateShaderModule(DEBUGSHADOWMAP_FRAG);
            vertShaderStageInfo.module = vertShaderModule;
            fragShaderStageInfo.module = fragShaderModule;
            VkPipelineShaderStageCreateInfo ShaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
            pipelineInfo.pStages = ShaderStages;
            device->CreateGraphicsPipelines(&pipelineInfo, &pipelines[1].pipeline);
            device->DestroyShaderModule(fragShaderModule);
            device->DestroyShaderModule(vertShaderModule);
        }
    }

    void CascadeShadowMapPass::updateCascade() {
        auto camera = engineGlobalContext.renderSystem->getRenderCamera();

        for (auto &cas: depthPass->needUpdate) {
            float lastSplitDist = cas == 0 ? 0 : cascadeSplits[cas - 1];
            float splitDist = cascadeSplits[cas];

            glm::vec3 frustumCorners[8] = {
                    glm::vec3(-1.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, -1.0f, 0.0f),
                    glm::vec3(-1.0f, -1.0f, 0.0f),
                    glm::vec3(-1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, -1.0f, 1.0f),
                    glm::vec3(-1.0f, -1.0f, 1.0f),
            };

            glm::mat4 invCam = glm::inverse(camera->matrices.perspective * camera->matrices.view);
            for (uint32_t i = 0; i < 8; i++) {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
                frustumCorners[i] = invCorner / invCorner.w;
            }

            for (uint32_t i = 0; i < 4; i++) {
                glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
                frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
                frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
            }

            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for (uint32_t i = 0; i < 8; i++) {
                frustumCenter += frustumCorners[i];
            }
            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for (uint32_t i = 0; i < 8; i++) {
                float distance = glm::length(frustumCorners[i] - frustumCenter);
                radius = glm::max(radius, distance);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;

            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;

            glm::vec3 lightDir = normalize(-renderResource->cameraObject.lightPos);
            glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter,
                                                    glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f,
                                                    maxExtents.z - minExtents.z);

            cascades[cas].splitDepth =
                    (camera->getNearClip() + splitDist * (camera->getFarClip() - camera->getNearClip())) * -1.0f;
            cascades[cas].lightProjViewMat = lightOrthoMatrix * lightViewMatrix;
        }
    }

    void CascadeShadowMapPass::draw() {
        auto commandBuffer = device->getCurrentCommandBuffer();
        vkCmdBindDescriptorSets(device->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].layout,
                                0, 1, &descriptors[0].descriptorSet, 0, nullptr);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0].pipeline);

        PushConstBlock pushConstant;
        engineGlobalContext.getScene()->draw(device->getCurrentCommandBuffer(), RenderFlags::BindImages,
                                             pipelines[0].layout, 1, &pushConstant, sizeof(pushConstant));
    }

    void CascadeShadowMapPass::setCascadeSplits() {

        auto camera = engineGlobalContext.renderSystem->getRenderCamera();
        float nearClip = camera->getNearClip();
        float farClip = camera->getFarClip();
        float clipRange = farClip - nearClip;

        float minZ = nearClip;
        float maxZ = nearClip + clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        for (uint32_t i = 0; i < CASCADE_COUNT; i++) {
            float p = (i + 1) * 1.f / CASCADE_COUNT;
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }
    }

    void CascadeShadowMapPass::drawDepth() {
        depthPass->drawLayer();
    }
    void CascadeShadowMapPass::createCSMGlobalDescriptor() {
        std::vector<VkDescriptorSetLayoutBinding> binding = {
                CreateDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pBindings = binding.data();
        descriptorSetLayoutCreateInfo.bindingCount = binding.size();
        device->CreateDescriptorSetLayout(&descriptorSetLayoutCreateInfo, &CSMGlobalDescriptor.layout);
        device->CreateDescriptorSet(1, CSMGlobalDescriptor.layout, CSMGlobalDescriptor.descriptorSet);
        VkDescriptorImageInfo imageInfos{};
        imageInfos.sampler = VK_NULL_HANDLE; //why NULL_HANDLE
        imageInfos.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos.imageView = fatherFramebuffer->attachments[main_camera_shadow].view;
        VkWriteDescriptorSet descriptorWrites{};

        descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites.dstSet = CSMGlobalDescriptor.descriptorSet;
        descriptorWrites.dstBinding = 0;
        descriptorWrites.dstArrayElement = 0;
        descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites.descriptorCount = 1;
        descriptorWrites.pImageInfo = &imageInfos;

        device->UpdateDescriptorSets(1, &descriptorWrites);
    }

    void CascadeShadowMapPass::updateAfterFramebufferRecreate() {
        createCSMGlobalDescriptor();
    }
}