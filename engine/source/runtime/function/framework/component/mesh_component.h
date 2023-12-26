#pragma once

#include "component.h"
#include "function/render/render_entity.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace MW {
    class MeshComponent : public Component {
    public:
        MeshComponent(const std::weak_ptr<GameObject> &parent, const std::string &type,
                      std::vector<Vertex> &_points)
                : Component(parent, type), points{_points} {}

        void getWorld();

        void getIndices();

        void setPoints(std::vector<glm::vec3> &_points);

        std::vector<Vertex> points{};
        std::vector<uint16_t> indices{};

        void tick() override;
    };
}