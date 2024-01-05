#pragma once

#include "function/render/rhi/vulkan_device.h"

#include <memory>
#include <vector>

namespace MW {
    class RenderResource;

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
