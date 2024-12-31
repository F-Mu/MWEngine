#include <map>
#include <stdexcept>
#include <array>
#include <iostream>
#include "main_camera_pass.h"
#include "mesh_base_pass.h"
#include "function/render/render_resource.h"
#include "shadow_passes/cascade_shadow_map_pass.h"
#include "shadow_passes/deferred_csm_pass.h"
#include "ao_passes/ssao_pass.h"
#include "pbr_ibl_passes/pbr_ibl_pass.h"
#include "function/global/engine_global_context.h"
#include "function/render/scene_manager.h"
#include "function/input/input_system.h"
#include "shading_pass.h"
#include "scene_frag.h"
#include "scene_vert.h"

namespace MW {
    extern VkDescriptorSetLayout descriptorSetLayoutImage;

    void MainCameraPass::initialize(const RenderPassInitInfo *init_info) {
        PassBase::initialize(init_info);
        createAttachments();
        createRenderPass();
//        createUniformBuffer();
//        createDescriptorSets();
//        createPipelines();
        createSwapchainFramebuffers();
        /* 注意顺序,gBuffer中有global的descriptorSet */
#if USE_MESH_SHADER
        gBufferPass = std::make_shared<MeshBaseBufferPass>();
#else
        gBufferPass = std::make_shared<GBufferPass>();
#endif
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

        lightingPass = std::make_shared<PbrIblPass>();
        PbrIblPassInitInfo pbrIblInfo(init_info);
        pbrIblInfo.frameBuffer = &framebuffer;
        lightingPass->initialize(&pbrIblInfo);

        shadingPass = std::make_shared<ShadingPass>();
        ShadingPassInitInfo shadingInfo(init_info);
        shadingInfo.frameBuffer = &framebuffer;
        shadingPass->initialize(&shadingInfo);

        UIPass = std::make_shared<UIOverlay>();
        UIPassInitInfo uiInfo(init_info);
        uiInfo.renderPass = framebuffer.renderPass;
        uiInfo.colorFormat = device->swapChainImageFormat;
        uiInfo.depthFormat = device->swapChainDepthFormat;
        uiInfo.queue = device->graphicsQueue;
        UIPass->initialize(&uiInfo);
    }

    void MainCameraPass::clean() {
//        UIPass->clean();
        shadingPass->clean();
        lightingPass->clean();
        ssaoPass->clean();
        shadowMapPass->clean();
        gBufferPass->clean();

//        UIPass.reset();
        shadingPass.reset();
        lightingPass.reset();
        ssaoPass.reset();
        shadowMapPass.reset();
        gBufferPass.reset();
        for (size_t i = 0; i < framebuffer.attachments.size(); i++) {
            device->DestroyImage(framebuffer.attachments[i].image);
            device->DestroyImageView(framebuffer.attachments[i].view);
            device->FreeMemory(framebuffer.attachments[i].mem);
        }
        device->DestroyRenderPass(framebuffer.renderPass);
        for (auto framebuffer: swapChainFramebuffers) {
            device->DestroyFramebuffer(framebuffer);
        }
        PassBase::clean();
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
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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
        std::array<VkSubpassDescription, main_camera_subpass_count> subpassDescs{};
        {
            // Create G-Buffer SubPass;
            std::array<VkAttachmentReference, 4> gBufferAttachmentReferences{};
            gBufferAttachmentReferences[g_buffer_material].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            gBufferAttachmentReferences[g_buffer_material].attachment = g_buffer_material;
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
        }
        {
            // Create CSM SubPass;
            std::array<VkAttachmentReference, 5> ScreenSpaceInputAttachmentReferences{};
            ScreenSpaceInputAttachmentReferences[g_buffer_material].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ScreenSpaceInputAttachmentReferences[g_buffer_material].attachment = g_buffer_material;
            ScreenSpaceInputAttachmentReferences[g_buffer_normal].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ScreenSpaceInputAttachmentReferences[g_buffer_normal].attachment = g_buffer_normal;
            ScreenSpaceInputAttachmentReferences[g_buffer_albedo].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ScreenSpaceInputAttachmentReferences[g_buffer_albedo].attachment = g_buffer_albedo;
            ScreenSpaceInputAttachmentReferences[g_buffer_view_position].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ScreenSpaceInputAttachmentReferences[g_buffer_view_position].attachment = g_buffer_view_position;
            ScreenSpaceInputAttachmentReferences[4].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ScreenSpaceInputAttachmentReferences[4].attachment = main_camera_depth;

            std::array<VkAttachmentReference, 1> CSMOutputAttachmentReferences{};
            CSMOutputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            CSMOutputAttachmentReferences[0].attachment = main_camera_shadow;

            subpassDescs[main_camera_subpass_csm_pass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescs[main_camera_subpass_csm_pass].inputAttachmentCount = ScreenSpaceInputAttachmentReferences.size();
            subpassDescs[main_camera_subpass_csm_pass].pInputAttachments = ScreenSpaceInputAttachmentReferences.data();
            subpassDescs[main_camera_subpass_csm_pass].colorAttachmentCount = CSMOutputAttachmentReferences.size();
            subpassDescs[main_camera_subpass_csm_pass].pColorAttachments = CSMOutputAttachmentReferences.data();
        }
        {
            // Create SSAO SubPass;
            std::array<VkAttachmentReference, 2> SSAOInputAttachmentReferences{};
            SSAOInputAttachmentReferences[g_buffer_material].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            SSAOInputAttachmentReferences[g_buffer_material].attachment = g_buffer_material;
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
        }
        {
            // Create Lighting SubPass;
            // TODO:FIX
            std::array<VkAttachmentReference, 5> LightingInputAttachmentReferences{};
            LightingInputAttachmentReferences[g_buffer_material].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            LightingInputAttachmentReferences[g_buffer_material].attachment = g_buffer_material;
            LightingInputAttachmentReferences[g_buffer_normal].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            LightingInputAttachmentReferences[g_buffer_normal].attachment = g_buffer_normal;
            LightingInputAttachmentReferences[g_buffer_albedo].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            LightingInputAttachmentReferences[g_buffer_albedo].attachment = g_buffer_albedo;
            LightingInputAttachmentReferences[g_buffer_view_position].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            LightingInputAttachmentReferences[g_buffer_view_position].attachment = g_buffer_view_position;
            LightingInputAttachmentReferences[4].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            LightingInputAttachmentReferences[4].attachment = main_camera_depth;

            std::array<VkAttachmentReference, 1> LightingOutputAttachmentReferences{};
            LightingOutputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            LightingOutputAttachmentReferences[0].attachment = main_camera_lighting;

            subpassDescs[main_camera_subpass_lighting_pass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescs[main_camera_subpass_lighting_pass].inputAttachmentCount = LightingInputAttachmentReferences.size();
            subpassDescs[main_camera_subpass_lighting_pass].pInputAttachments = LightingInputAttachmentReferences.data();
            subpassDescs[main_camera_subpass_lighting_pass].colorAttachmentCount = LightingOutputAttachmentReferences.size();
            subpassDescs[main_camera_subpass_lighting_pass].pColorAttachments = LightingOutputAttachmentReferences.data();
        }
        {
            // Create Shading SubPass;
            std::array<VkAttachmentReference, 3> ShadingInputAttachmentReferences{};
            ShadingInputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ShadingInputAttachmentReferences[0].attachment = main_camera_shadow;
            ShadingInputAttachmentReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ShadingInputAttachmentReferences[1].attachment = main_camera_ao;
            ShadingInputAttachmentReferences[2].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ShadingInputAttachmentReferences[2].attachment = main_camera_lighting;

            std::array<VkAttachmentReference, 1> ShadingOutputAttachmentReferences{};
            ShadingOutputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ShadingOutputAttachmentReferences[0].attachment = main_camera_swap_chain_image;
            subpassDescs[main_camera_subpass_shading_pass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescs[main_camera_subpass_shading_pass].inputAttachmentCount = ShadingInputAttachmentReferences.size();
            subpassDescs[main_camera_subpass_shading_pass].pInputAttachments = ShadingInputAttachmentReferences.data();
            subpassDescs[main_camera_subpass_shading_pass].colorAttachmentCount = ShadingOutputAttachmentReferences.size();
            subpassDescs[main_camera_subpass_shading_pass].pColorAttachments = ShadingOutputAttachmentReferences.data();
        }
        {
            // Create UI SubPass;
            std::array<VkAttachmentReference, 1> UIOutputAttachmentReferences{};
            UIOutputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            UIOutputAttachmentReferences[0].attachment = main_camera_swap_chain_image;
            subpassDescs[main_camera_subpass_ui_pass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescs[main_camera_subpass_ui_pass].colorAttachmentCount = UIOutputAttachmentReferences.size();
            subpassDescs[main_camera_subpass_ui_pass].pColorAttachments = UIOutputAttachmentReferences.data();
        }
        std::array<VkSubpassDependency, 8> dependencies = {};
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

        dependencies[5].srcSubpass = main_camera_subpass_g_buffer_pass;
        dependencies[5].dstSubpass = main_camera_subpass_lighting_pass;
        dependencies[5].srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[5].dstStageMask =
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[5].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[5].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependencies[5].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[6].srcSubpass = main_camera_subpass_lighting_pass;
        dependencies[6].dstSubpass = main_camera_subpass_shading_pass;
        dependencies[6].srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[6].dstStageMask =
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[6].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[6].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependencies[6].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


        dependencies[7].srcSubpass = main_camera_subpass_shading_pass;
        dependencies[7].dstSubpass = main_camera_subpass_ui_pass;
        dependencies[7].srcStageMask =
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[7].dstStageMask =
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[7].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[7].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependencies[7].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

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
                    framebuffer.attachments[g_buffer_material].view,
                    framebuffer.attachments[g_buffer_normal].view,
                    framebuffer.attachments[g_buffer_albedo].view,
                    framebuffer.attachments[g_buffer_view_position].view,
                    framebuffer.attachments[main_camera_shadow].view,
                    framebuffer.attachments[main_camera_ao].view,
                    framebuffer.attachments[main_camera_lighting].view,
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
        clearColors[main_camera_depth].depthStencil = {1.0f, 0};
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

        lightingPass->draw();

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
        vkCmdNextSubpass(device->getCurrentCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);
        drawUI(device->getCurrentCommandBuffer());
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
        shadowMapPass->updateAfterFramebufferRecreate();
        ssaoPass->updateAfterFramebufferRecreate();
        lightingPass->updateAfterFramebufferRecreate();
    }

    void MainCameraPass::updateCamera() {
        memcpy(cameraUniformBuffer.mapped, &renderResource->cameraObject, sizeof(renderResource->cameraObject));
    }

    void MainCameraPass::preparePassData() {
//        updateCamera();
        gBufferPass->preparePassData();
        shadowMapPass->preparePassData();
        ssaoPass->preparePassData();
        lightingPass->preparePassData();
        shadingPass->preparePassData();
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
                         &framebuffer.attachments[g_buffer_material]);

        // (World space) Normals
        createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[g_buffer_normal]);

        // Albedo (color)
        createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[g_buffer_albedo]);

        // Camera space position
        createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[g_buffer_view_position]);

        // Shadow (color)
        createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[main_camera_shadow]);

        // SSAO (color)
        createAttachment(VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[main_camera_ao]);
        // IBL (color)
        createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         &framebuffer.attachments[main_camera_lighting]);
    }

    void MainCameraPass::drawUI(const VkCommandBuffer commandBuffer) {
//        VkViewport viewport{};
//        viewport.x = 0.0f;
//        viewport.y = 0.0f;
//        viewport.width = (float) device->getSwapchainInfo().extent.width;
//        viewport.height = (float) device->getSwapchainInfo().extent.height;
//        viewport.minDepth = 0.0f;
//        viewport.maxDepth = 1.0f;
//        vkCmdSetViewport(device->getCurrentCommandBuffer(), 0, 1, &viewport);
//        vkCmdSetScissor(device->getCurrentCommandBuffer(), 0, 1, device->getSwapchainInfo().scissor);
        UIPass->draw(commandBuffer);
    }

    void MainCameraPass::updateOverlay(float delta_time) {
        // The overlay does not need to be updated with each frame, so we limit the update rate
        // Not only does this save performance but it also makes display of fast changig values like fps more stable
        UIPass->updateTimer -= delta_time;
        if (UIPass->updateTimer >= 0.0f) {
            return;
        }
        // Update at max. rate of 30 fps
        UIPass->updateTimer = 1.0f / 30.0f;

        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float) device->getSwapchainInfo().extent.width,
                                (float) device->getSwapchainInfo().extent.height);
        io.DeltaTime = delta_time;

        auto mouseState = engineGlobalContext.inputSystem->getMouseState();
        io.MousePos = ImVec2(mouseState.position.x, mouseState.position.y);
        io.MouseDown[0] = mouseState.buttons.left;
        io.MouseDown[1] = mouseState.buttons.right;
        io.MouseDown[2] = mouseState.buttons.middle;

        ImGui::NewFrame();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("MW Engine", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::TextUnformatted(device->properties.deviceName);
        ImGui::PushItemWidth(110.0f * UIPass->scale);
//        OnUpdateUIOverlay(&UI);
        ImGui::PopItemWidth();

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::Render();
        UIPass->update();
    }

    void MainCameraPass::processAfterPass() {
        UIPass->update();
    }
}