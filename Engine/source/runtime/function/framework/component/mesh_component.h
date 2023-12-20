#pragma once

#include "render/render_device.h"
#include "component.h"
#include "core/vertex.h"
#include <glm/glm.h>
#include <vector>

namespace MW {
    class MeshComponent : public Component {
    public:
        MeshComponent(const std::weak_ptr<GameObject> &parent, const std::string &type,
                      std::vector<glm::vec3> &_points, glm::vec3 &_color, bool _fill);

        void getWorld();

        void getIndices();

        void setPoints(std::vector<glm::vec3> &_points);

        std::vector<glm::vec3> points{};
        std::vector<Vertex> vertexPoints{};
        std::vector<uint32_t> worldIndices{};
        glm::vec3 color;
        bool fill;

        void tick() override;
    };
}