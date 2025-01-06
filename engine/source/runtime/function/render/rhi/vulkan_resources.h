#pragma once

#define USE_MESH_SHADER 1
#if USE_MESH_SHADER
#define NV_MESH_SHADER 0
#define EXT_MESH_SHADER 1
#endif
#if !NV_MESH_SHADER && !EXT_MESH_SHADER
#define USE_MESH_SHADER 0
#endif
#if NV_MESH_SHADER & EXT_MESH_SHADER
#define USE_MESH_SHADER 0
#endif
#define USE_VRS 1
#define USE_RAY_TRACING_SCENE 0
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <string>
#include "ktx.h"

namespace MW {
    class VulkanDevice;

    struct VulkanBuffer {
        VkDescriptorBufferInfo descriptor;
        void *mapped = nullptr;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkDeviceSize bufferSize;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;

        void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    };

    struct VulkanTexture {
        VkImage image;
        VkImageLayout imageLayout;
        VkDeviceMemory deviceMemory;
        VkImageView view;
        uint32_t width, height;
        uint32_t mipLevels;
        uint32_t layerCount;
        VkDescriptorImageInfo descriptor;
        VkSampler sampler;

        void updateDescriptor();

        void destroy(std::shared_ptr<VulkanDevice> device);

        ktxResult loadKTXFile(std::string filename, ktxTexture **target);
    };

    struct VulkanTexture2D : public VulkanTexture {
        void fromBuffer(
                void *buffer,
                VkDeviceSize bufferSize,
                VkFormat format,
                uint32_t texWidth,
                uint32_t texHeight,
                std::shared_ptr<VulkanDevice> device,
                VkFilter filter = VK_FILTER_LINEAR,
                VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
                VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    };

    class VulkanTextureCubeMap : public VulkanTexture {
    public:
        void loadFromFile(
                std::string filename,
                VkFormat format,
                std::shared_ptr<VulkanDevice> device,
                VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
                VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    };

    struct VulkanMesh {
        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;
    };

    struct VulkanSwapChainDesc {
        VkExtent2D extent;
        VkFormat imageFormat;
        VkViewport *viewport;
        VkRect2D *scissor;
        std::vector<VkImageView> imageViews;
        std::vector<VkImage> images;
    };

    struct VulkanDepthImageDesc {
        VkImage depthImage = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;
        VkFormat depthImageFormat;
    };
#if USE_VRS
    struct VulkanShadingRateImageDesc {
        VkExtent2D extent;
        VkImage shadingRateImage = VK_NULL_HANDLE;
        VkImageView shadingRateImageView = VK_NULL_HANDLE;
        VkFormat shadingRateImageFormat;
    };
#endif
    struct RayTracingScratchBuffer {
        uint64_t deviceAddress = 0;
        VkBuffer handle = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    // Ray tracing acceleration structure
    struct AccelerationStructure {
        VkAccelerationStructureKHR handle;
        uint64_t deviceAddress = 0;
        VkDeviceMemory memory;
        VkBuffer buffer;
        void destroy(std::shared_ptr<VulkanDevice> device);
    };

    struct StorageImage {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFormat format;
        void destroy(std::shared_ptr<VulkanDevice> device);
    };
}  // namespace MW