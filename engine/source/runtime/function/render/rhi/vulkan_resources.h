#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

namespace MW {

    struct VulkanBuffer {
        void *mapped = nullptr;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkDeviceSize bufferSize;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;
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

    struct RayTracingScratchBuffer
    {
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
    };

    struct StorageImage {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFormat format;
    };
}  // namespace MW