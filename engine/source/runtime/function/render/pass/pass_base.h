#pragma once

#include "function/render/rhi/vulkan_device.h"

#include <memory>
#include <vector>

namespace MW {
    class RenderResource;
    enum {
        g_buffer_material = 0,
        g_buffer_normal = 1,
        g_buffer_albedo = 2,
        g_buffer_view_position = 3,
        main_camera_g_buffer_type_count = 4,
        //
        main_camera_shadow = 4,
        main_camera_ao = 5,
        main_camera_lighting = 6,
        main_camera_custom_type_count = 3,
        //
        main_camera_depth = 7,
#if USE_VRS
        main_camera_shading_rate_image = 8,
#endif
        main_camera_swap_chain_image = main_camera_depth + USE_VRS + 1,
        main_camera_device_type_count = 2 + USE_VRS,
        //
        main_camera_post_process_type_count = 0,
        main_camera_type_count = 9 + USE_VRS
    };

    enum {
        main_camera_subpass_g_buffer_pass = 0,
        main_camera_subpass_csm_pass = 1,
        main_camera_subpass_ssao_pass = 2,
        main_camera_subpass_lighting_pass = 3,
        main_camera_subpass_shading_pass = 4,
        main_camera_subpass_ui_pass = 5,
        main_camera_subpass_count = 6
    };
    struct RenderPassInitInfo {
        std::shared_ptr<VulkanDevice> device;
        std::shared_ptr<RenderResource> renderResource;
    };

    constexpr uint32_t DEFAULT_IMAGE_WIDTH = 4096;
    constexpr uint32_t DEFAULT_IMAGE_HEIGHT = 4096;

    class PassBase {
    public:
        struct FrameBufferAttachment {
            VkImage image;
            VkDeviceMemory mem;
            VkImageView view;
            VkFormat format;
        };

        struct Framebuffer {
            int width;
            int height;
            VkFramebuffer framebuffer;
            VkRenderPass renderPass;

            std::vector<FrameBufferAttachment> attachments;
        };

        struct Descriptor {
            VkDescriptorSetLayout layout;
            VkDescriptorSet descriptorSet;
        };

        struct RenderPipelineBase {
            VkPipelineLayout layout;
            VkPipeline pipeline;
        };

        std::vector<Descriptor> descriptors;
        std::vector<RenderPipelineBase> pipelines;
        Framebuffer framebuffer;

        virtual void draw();

        virtual VkRenderPass getRenderPass() const;

        virtual std::vector<VkImageView> getFramebufferImageViews() const;

        virtual std::vector<VkDescriptorSetLayout> getDescriptorSetLayouts() const;

        virtual void initialize(const RenderPassInitInfo *info);

        virtual void clean();

        virtual void preparePassData();

    protected:
        std::shared_ptr<VulkanDevice> device;
        std::shared_ptr<RenderResource> renderResource;
    };
} // namespace MW
