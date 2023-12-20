#pragma once

#include <glm/glm.h>
#include "component.h"
#include "string"
#include "core/macro.h"

namespace MW {
    class TransformComponent : public Component {
    public:
        explicit TransformComponent(const std::weak_ptr<GameObject> &parent, const std::string &type) :
                Component(parent, type) {}

        glm::vec3 translation{};
        glm::vec3 scale{SCALE};
        glm::vec3 rotation{};

        // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
        // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
        // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
        glm::mat4 mat4();

        glm::mat3 normalMatrix();

        void setPosition(glm::vec3 point);

        void tick() override;
    };
}