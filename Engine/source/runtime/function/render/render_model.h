#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "core/vertex.h"
#include "render/render_buffer.h"

namespace MW {
    class Model {
    public:
        Model(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

        Model(const std::vector<Vertex> &vertices);

        ~Model();

        Model() = default;

        Model(const Model &) = default;

        Model &operator=(const Model &) = default;

        void bind(VkCommandBuffer commandBuffer);

        void draw(VkCommandBuffer commandBuffer) const;

//    private:
        void createVertexBuffers(const std::vector<Vertex> &vertices);

        void createIndexBuffers(const std::vector<uint32_t> &indices);

        std::shared_ptr<RenderBuffer> vertexBuffer;
        uint32_t vertexCount;

        bool hasIndexBuffer = false;
        std::shared_ptr<RenderBuffer> indexBuffer;
        uint32_t indexCount;
    };
}