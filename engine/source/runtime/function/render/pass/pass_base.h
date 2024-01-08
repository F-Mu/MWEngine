#pragma once

#include "function/render/rhi/vulkan_device.h"

#include <memory>
#include <vector>

namespace MW {
    class RenderResource;
    enum {
        g_buffer_position = 0,
        g_buffer_normal = 1,
        g_buffer_albedo = 2,
        g_buffer_view_position = 3,
        main_camera_g_buffer_type_count = 4,
        main_camera_custom_type_count = 4,
        //
        main_camera_depth = 4,
        main_camera_swap_chain_image = 5,
        //
        main_camera_post_process_type_count = 0,
        main_camera_type_count = 6
    };

    enum {
        main_camera_subpass_g_buffer_pass = 0,
        main_camera_subpass_csm_pass = 1,
        main_camera_subpass_count
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

        virtual void preparePassData();

    protected:
        std::shared_ptr<VulkanDevice> device;
        std::shared_ptr<RenderResource> renderResource;
    };
} // namespace MW
