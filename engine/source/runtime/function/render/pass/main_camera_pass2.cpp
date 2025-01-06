#include "main_camera_pass2.h"
#include "runtime/function/render/pass/base_passes/mesh_base_pass.h"
#include "function/render/render_resource.h"
#include "shadow_passes/cascade_shadow_map_pass.h"
#include "shadow_passes/deferred_csm_pass.h"
#include "ao_passes/ssao_pass.h"
#include "pbr_ibl_passes/pbr_ibl_pass.h"
#include "function/global/engine_global_context.h"
#include "function/render/scene_manager.h"
#include "function/input/input_system.h"
#include "shading_pass.h"

namespace MW {
#if USE_VRS
    void MainCameraPass2::initialize(const RenderPassInitInfo *init_info) {
        MainCameraPass::initialize(init_info);
        registerOnUIFunc(std::bind(&MainCameraPass2::UpdateUIOverlay, this, std::placeholders::_1));
    }

    void MainCameraPass2::createRenderPass() {
        std::array<VkAttachmentDescription2KHR, main_camera_type_count> attachments{};

        /* 假设所有attachment的部分格式是一样的 */
        for (auto &attachmentDesc: attachments) {
            attachmentDesc.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2_KHR;
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

        VkAttachmentDescription2KHR &depthAttachment = attachments[main_camera_depth];
        depthAttachment.format = device->getDepthImageInfo().depthImageFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription2KHR &colorAttachment = attachments[main_camera_swap_chain_image];
        colorAttachment.format = device->getSwapchainInfo().imageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription2KHR &shadingRateAttachment = attachments[main_camera_shading_rate_image];
        shadingRateAttachment.format = device->getShadingRateImageInfo().shadingRateImageFormat;
        shadingRateAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        shadingRateAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        shadingRateAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        shadingRateAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        shadingRateAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        shadingRateAttachment.initialLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        shadingRateAttachment.finalLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

        // Setup the attachment reference for the shading rate image attachment in slot 2
        VkAttachmentReference2KHR fragmentShadingRateReference{};
        fragmentShadingRateReference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        fragmentShadingRateReference.attachment = main_camera_shading_rate_image;
        fragmentShadingRateReference.layout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

        // Setup the attachment info for the shading rate image, which will be added to the sub pass via structure chaining (in pNext)
        VkFragmentShadingRateAttachmentInfoKHR fragmentShadingRateAttachmentInfo{};
        fragmentShadingRateAttachmentInfo.sType = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
        fragmentShadingRateAttachmentInfo.pFragmentShadingRateAttachment = &fragmentShadingRateReference;
        fragmentShadingRateAttachmentInfo.shadingRateAttachmentTexelSize.width = device->physicalDeviceShadingRateImageProperties.maxFragmentShadingRateAttachmentTexelSize.width;
        fragmentShadingRateAttachmentInfo.shadingRateAttachmentTexelSize.height = device->physicalDeviceShadingRateImageProperties.maxFragmentShadingRateAttachmentTexelSize.height;

        std::array<VkSubpassDescription2KHR, main_camera_subpass_count> subpassDescs{};
        for (auto &description: subpassDescs) {
            description.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
            description.pNext = &fragmentShadingRateAttachmentInfo;
        }
        {
            // Create G-Buffer SubPass;
            std::array<VkAttachmentReference2KHR, 4> gBufferAttachmentReferences{};
            setupAttachment2KHRType(gBufferAttachmentReferences);
            gBufferAttachmentReferences[g_buffer_material].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            gBufferAttachmentReferences[g_buffer_material].attachment = g_buffer_material;
            gBufferAttachmentReferences[g_buffer_normal].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            gBufferAttachmentReferences[g_buffer_normal].attachment = g_buffer_normal;
            gBufferAttachmentReferences[g_buffer_albedo].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            gBufferAttachmentReferences[g_buffer_albedo].attachment = g_buffer_albedo;
            gBufferAttachmentReferences[g_buffer_view_position].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            gBufferAttachmentReferences[g_buffer_view_position].attachment = g_buffer_view_position;

            VkAttachmentReference2KHR depthAttachmentReference{};
            depthAttachmentReference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR;
            depthAttachmentReference.attachment = main_camera_depth;
            depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachmentReference.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            subpassDescs[main_camera_subpass_g_buffer_pass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescs[main_camera_subpass_g_buffer_pass].colorAttachmentCount = gBufferAttachmentReferences.size();
            subpassDescs[main_camera_subpass_g_buffer_pass].pColorAttachments = gBufferAttachmentReferences.data();
            subpassDescs[main_camera_subpass_g_buffer_pass].pDepthStencilAttachment = &depthAttachmentReference;
        }
        {
            // Create CSM SubPass;
            std::array<VkAttachmentReference2KHR, 5> ScreenSpaceInputAttachmentReferences{};
            setupAttachment2KHRType(ScreenSpaceInputAttachmentReferences);
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
            ScreenSpaceInputAttachmentReferences[4].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            std::array<VkAttachmentReference2KHR, 1> CSMOutputAttachmentReferences{};
            setupAttachment2KHRType(CSMOutputAttachmentReferences);
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
            std::array<VkAttachmentReference2KHR, 2> SSAOInputAttachmentReferences{};
            setupAttachment2KHRType(SSAOInputAttachmentReferences);
            SSAOInputAttachmentReferences[g_buffer_material].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            SSAOInputAttachmentReferences[g_buffer_material].attachment = g_buffer_material;
            SSAOInputAttachmentReferences[g_buffer_normal].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            SSAOInputAttachmentReferences[g_buffer_normal].attachment = g_buffer_normal;

            std::array<VkAttachmentReference2KHR, 1> SSAOOutputAttachmentReferences{};
            setupAttachment2KHRType(SSAOOutputAttachmentReferences);
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
            std::array<VkAttachmentReference2KHR, 5> LightingInputAttachmentReferences{};
            setupAttachment2KHRType(LightingInputAttachmentReferences);
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
            LightingInputAttachmentReferences[4].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            std::array<VkAttachmentReference2KHR, 1> LightingOutputAttachmentReferences{};
            setupAttachment2KHRType(LightingOutputAttachmentReferences);
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
            std::array<VkAttachmentReference2KHR, 3> ShadingInputAttachmentReferences{};
            setupAttachment2KHRType(ShadingInputAttachmentReferences);
            ShadingInputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ShadingInputAttachmentReferences[0].attachment = main_camera_shadow;
            ShadingInputAttachmentReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ShadingInputAttachmentReferences[1].attachment = main_camera_ao;
            ShadingInputAttachmentReferences[2].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ShadingInputAttachmentReferences[2].attachment = main_camera_lighting;

            std::array<VkAttachmentReference2KHR, 1> ShadingOutputAttachmentReferences{};
            setupAttachment2KHRType(ShadingOutputAttachmentReferences);
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
            std::array<VkAttachmentReference2KHR, 1> UIOutputAttachmentReferences{};
            setupAttachment2KHRType(UIOutputAttachmentReferences);
            UIOutputAttachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            UIOutputAttachmentReferences[0].attachment = main_camera_swap_chain_image;
            subpassDescs[main_camera_subpass_ui_pass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescs[main_camera_subpass_ui_pass].colorAttachmentCount = UIOutputAttachmentReferences.size();
            subpassDescs[main_camera_subpass_ui_pass].pColorAttachments = UIOutputAttachmentReferences.data();
            subpassDescs[main_camera_subpass_ui_pass].pNext = VK_NULL_HANDLE;
        }
        std::array<VkSubpassDependency2KHR, 8> dependencies = {};
        for (auto &dependency: dependencies)
            dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
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

        VkRenderPassCreateInfo2KHR renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = subpassDescs.size();
        renderPassInfo.pSubpasses = subpassDescs.data();
        renderPassInfo.dependencyCount = dependencies.size();
        renderPassInfo.pDependencies = dependencies.data();

        device->CreateRenderPass2KHR(&renderPassInfo, &framebuffer.renderPass);
    }

    void MainCameraPass2::createSwapchainFramebuffers() {
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
                    device->getShadingRateImageInfo().shadingRateImageView,
                    device->getSwapchainInfo().imageViews[i]
            };

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

    template<size_t N>
    void MainCameraPass2::setupAttachment2KHRType(std::array<VkAttachmentReference2KHR, N> &attachmentReferences) {
        for (auto &reference: attachmentReferences) {
            reference.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            reference.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    void MainCameraPass2::draw() {
        shadowMapPass->drawDepth();
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = framebuffer.renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[device->currentImageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = device->getSwapchainInfo().extent;

        std::array<VkClearValue, main_camera_g_buffer_type_count + main_camera_custom_type_count +
                                 main_camera_device_type_count> clearColors{};
        for (int i = 0; i < main_camera_g_buffer_type_count + main_camera_custom_type_count; ++i)
            clearColors[i] = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearColors[main_camera_depth].depthStencil = {1.0f, 0};
        clearColors[main_camera_shading_rate_image] = {{0.0f, 0.0f, 0.0f, 10.0f}};
        clearColors[main_camera_swap_chain_image] = {{0.0f, 0.0f, 0.0f, 1.0f}};
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

        device->vkCmdSetFragmentShadingRateKHR(device->getCurrentCommandBuffer(), &fragmentShadingRateSize,
                                               combinerOps);

        gBufferPass->draw();

        vkCmdNextSubpass(device->getCurrentCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);

        shadowMapPass->draw();
        vkCmdNextSubpass(device->getCurrentCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);

        ssaoPass->draw();
        vkCmdNextSubpass(device->getCurrentCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);

        lightingPass->draw();

        vkCmdNextSubpass(device->getCurrentCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);

        shadingPass->draw();
        vkCmdNextSubpass(device->getCurrentCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);
        drawUI(device->getCurrentCommandBuffer());
        vkCmdEndRenderPass(device->getCurrentCommandBuffer());
    }

    void MainCameraPass2::UpdateUIOverlay(UIOverlay *overlay) {
        overlay->header("VRS Settings");
        if (overlay->comboBox("Pipeline Rate X", &shadingRateIndex.x, {"1", "2", "4"})) {
            if (shadingRateIndex.x == 0)
                fragmentShadingRateSize.height = 1;
            else if (shadingRateIndex.x == 1)
                fragmentShadingRateSize.height = 2;
            else if (shadingRateIndex.x == 2)
                fragmentShadingRateSize.height = 4;
        }
        if (overlay->comboBox("Pipeline Rate Y", &shadingRateIndex.y, {"1", "2", "4"})) {
            if (shadingRateIndex.y == 0)
                fragmentShadingRateSize.width = 1;
            else if (shadingRateIndex.y == 1)
                fragmentShadingRateSize.width = 2;
            else if (shadingRateIndex.y == 2)
                fragmentShadingRateSize.width = 4;
        }
        std::vector<std::string> combiners = {
                "KEEP",
                "REPLACE",
                "MIN",
                "MAX",
                "MUL"
        };
        // If shading rate from attachment is enabled, we set the combiner, so that the values from the attachment are used
        // Combiner for pipeline (A) and primitive (B) - Not used in this sample
//        combinerOps[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
        // Combiner for pipeline (A) and attachment (B), replace the pipeline default value (fragment_size) with the fragment sizes stored in the attachment
//        combinerOps[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;

        if (overlay->comboBox("Pipeline & Primitive", &shadingRateCombinerIndex.x, combiners)) {
            combinerOps[0] = static_cast<VkFragmentShadingRateCombinerOpKHR>(shadingRateCombinerIndex.x);
        }
        if (overlay->comboBox("Pipeline & Attachment", &shadingRateCombinerIndex.y, combiners)) {
            combinerOps[1] = static_cast<VkFragmentShadingRateCombinerOpKHR>(shadingRateCombinerIndex.y);
        }
    }

#endif
} // MW