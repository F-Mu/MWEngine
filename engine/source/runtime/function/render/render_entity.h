#pragma once

#include "rhi/vulkan_resources.h"
#include "core/math/math_headers.h"
#include "function/render/render_resource.h"

namespace MW {
    struct Vertex {
    public:
        Vector3 position{};
        Vector3 color{};
        Vector3 normal{};
        Vector2 uv{};

        explicit Vertex(Vector3 _position = {}, Vector3 _color = {}, Vector3 _normal = {}, Vector2 _uv = {});

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
            std::vector<VkVertexInputBindingDescription> bindingDescriptions(4);

            // position
            bindingDescriptions[0].binding = 0;
            bindingDescriptions[0].stride = sizeof(Vector3);
            bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            // color
            bindingDescriptions[1].binding = 1;
            bindingDescriptions[1].stride = sizeof(Vector3);
            bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            // normal
            bindingDescriptions[2].binding = 2;
            bindingDescriptions[2].stride = sizeof(Vector3);
            bindingDescriptions[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            // uv
            bindingDescriptions[2].binding = 3;
            bindingDescriptions[2].stride = sizeof(Vector2);
            bindingDescriptions[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescriptions;
        }

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

            attributeDescriptions.push_back(
                    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, position))});
            attributeDescriptions.push_back(
                    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, color))});
            attributeDescriptions.push_back(
                    {2, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, normal))});
            attributeDescriptions.push_back(
                    {3, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, uv))});
            return attributeDescriptions;
        }
    };

    class RenderTexture {
    public:
        uint32_t width{0};
        uint32_t height{0};
        uint32_t depth{0};
        uint32_t mipLevels{0};
        uint32_t arrayLayers{0};
        void *pixels{nullptr};

        VkFormat format = VK_FORMAT_MAX_ENUM;

        RenderTexture() = default;

        ~RenderTexture() {
            if (pixels) {
                free(pixels);
            }
        }

        bool isValid() const { return pixels != nullptr; }
    };

    class RenderMaterial {
        std::shared_ptr<RenderTexture> baseColorTexture;

    };

    class RenderEntity {
    public:
        int vertexCount = 0;
        int indexCount = 0;
        VulkanMesh mesh;
        uint32_t materialId{0}; //base material
        std::vector<uint32_t> textureIds; //extra texture
    };

    class GLTFRenderEntity {
    };
}