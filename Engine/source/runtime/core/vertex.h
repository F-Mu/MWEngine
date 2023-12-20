#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace MW {

    struct Vertex {
    public:
        glm::vec3 position{};
        glm::vec3 color{};
        glm::vec3 normal{};
        glm::vec2 uv{};

        explicit Vertex(glm::vec3 _position = {}, glm::vec3 _color = {}, glm::vec3 _normal = {}, glm::vec2 _uv = {});

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        bool operator==(const Vertex &other) const {
            return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
        }
    };
}