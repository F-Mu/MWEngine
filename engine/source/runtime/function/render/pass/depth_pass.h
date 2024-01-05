#pragma once

#include "pass_base.h"

namespace MW {
    struct DepthPassInitInfo : public RenderPassInitInfo {
        // Maybe More
        uint32_t depthImageWidth{DEFAULT_IMAGE_WIDTH};
        uint32_t depthImageHeight{DEFAULT_IMAGE_HEIGHT};
        uint32_t depthArrayLayers{1};
    };
    struct PushConstBlock {
        glm::vec4 position; //深度测试的相机位置，也许是相机，也许是光源
    };
    struct UniformBufferObject {
        glm::mat4 viewProjMatrix;
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
    } depth;

    class DepthPass : public PassBase {
    public:
        void drawLayer(int nowLayer = 0);

        void initialize(const RenderPassInitInfo *info) override;

        PushConstBlock pushConstant;
        UniformBufferObject uniformBufferObject;
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
        DepthImage depth;
        std::vector<VkFramebuffer> depthFramebuffers;
        VulkanBuffer uniformBuffer;
    };
}