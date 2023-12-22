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

    struct Vertex {
    public:
        glm::vec3 position{};
        glm::vec3 normal{};
        glm::vec2 uv{};

        explicit Vertex(glm::vec3 _position = {}, glm::vec3 _color = {}, glm::vec3 _normal = {}, glm::vec2 _uv = {});

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
            std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
            bindingDescriptions[0].binding = 0;
            bindingDescriptions[0].stride = sizeof(Vertex);
            bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescriptions;
        }

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

            attributeDescriptions.push_back(
                    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, position))});
            attributeDescriptions.push_back(
                    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, normal))});
            attributeDescriptions.push_back(
                    {2, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, uv))});
            return attributeDescriptions;
        }

        bool operator==(const Vertex &other) const {
            return position == other.position && normal == other.normal && uv == other.uv;
        }
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
    };

    struct VulkanDepthImageDesc {
        VkImage depthImage = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;
        VkFormat depthImageFormat;
    };


}  // namespace MW