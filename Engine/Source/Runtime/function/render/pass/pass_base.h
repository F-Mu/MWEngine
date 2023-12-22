#pragma once

#include "function/render/rhi/vulkan_device.h"

#include <memory>
#include <vector>

namespace MW {

    struct RenderPassInitInfo {
        std::shared_ptr<VulkanDevice> device;
    };

    class RenderResource;

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
        std::vector<RenderPipelineBase> renderPipelines;
        Framebuffer framebuffer;

        virtual void draw();

        virtual VkRenderPass getRenderPass() const;

        virtual std::vector<VkImageView> getFramebufferImageViews() const;

        virtual std::vector<VkDescriptorSetLayout *> getDescriptorSetLayouts() const;

        virtual void preparePassData(std::shared_ptr<RenderResource> render_resource);

        virtual void initialize(const RenderPassInitInfo *info);

    protected:
        std::shared_ptr<VulkanDevice> device;
    };
} // namespace MW
