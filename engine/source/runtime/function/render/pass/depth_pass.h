#pragma once

#include "pass_base.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
namespace MW {
    struct DepthPassInitInfo : public RenderPassInitInfo {
        // Maybe More
        uint32_t depthImageWidth{DEFAULT_IMAGE_WIDTH};
        uint32_t depthImageHeight{DEFAULT_IMAGE_HEIGHT};
        uint32_t depthArrayLayers{1};
    };

    struct UniformBufferObject {
        glm::mat4 projViewMatrix;
    };

    struct DepthImage {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
        VkSampler sampler;

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