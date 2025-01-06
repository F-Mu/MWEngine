#pragma once

#include "runtime/function/render/pass/pass_base/pass_base.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "glm/glm.hpp"

namespace MW {
    struct DepthPassInitInfo : public RenderPassInitInfo {
        // Maybe More
        uint32_t depthImageWidth{DEFAULT_IMAGE_WIDTH};
        uint32_t depthImageHeight{DEFAULT_IMAGE_HEIGHT};
        uint32_t depthArrayLayers{1};

        explicit DepthPassInitInfo(const RenderPassInitInfo *info) : RenderPassInitInfo(*info) {};
    };

    struct UniformBufferObject {
        glm::mat4 projViewMatrix;
    };

    struct DepthImage {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
        VkSampler sampler;
        VkDescriptorImageInfo descriptor;

        void setDescriptor() {
            descriptor.sampler = sampler;
            descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            descriptor.imageView = view;
        }

        void destroy(VkDevice device) {
            vkDestroyImageView(device, view, nullptr);
            vkDestroyImage(device, image, nullptr);
            vkFreeMemory(device, mem, nullptr);
            vkDestroySampler(device, sampler, nullptr);
        }
    };

    class DepthPass : public PassBase {
    public:
        void drawLayer();

        void initialize(const RenderPassInitInfo *info) override;

        void clean() override;

        std::vector<UniformBufferObject> uniformBufferObjects;

        void preparePassData() override;

        std::vector<int> needUpdate;
        DepthImage depth;
    private:
        void draw() override;

        void createRenderPass();

        void createUniformBuffer();

        void createDescriptorSets();

        void createFramebuffers();

        void createPipelines();

        uint32_t nowArrayLayer;
        uint32_t depthImageWidth;
        uint32_t depthImageHeight;
        uint32_t depthArrayLayers;
        std::vector<VkFramebuffer> depthFramebuffers;
        std::vector<VulkanBuffer> uniformBuffers;
    };
}