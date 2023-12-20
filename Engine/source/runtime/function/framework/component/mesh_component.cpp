#include "mesh_component.h"
#include "transform_component.h"
#include "function/framework/game_object.h"
#include "utils/geometry.h"

namespace MW {
    MeshComponent::MeshComponent(const std::weak_ptr<GameObject> &parent, const std::string &type,
                                 std::vector<glm::vec3> &_points,
                                 glm::vec3 &_color, bool _fill)
            : Component(parent, type), points{_points}, color{_color}, fill{_fill} {
        getWorld();
    }

    void MeshComponent::tick() {
        getWorld();
    }

    void MeshComponent::getWorld() {
        if (fill) {
            vertexPoints = std::move(Geometry::fillMesh(points, color));
        } else {
            vertexPoints = std::move(Geometry::unfilledMesh(points, color));
        }
        getIndices();
    }

    void MeshComponent::getIndices() {
        worldIndices.clear();
        for (int i = 0; i + 2 < vertexPoints.size(); ++i) {
            for (int j = i; j <= i + 2; ++j)worldIndices.emplace_back(j);
        }
    }

    void MeshComponent::setPoints(std::vector<glm::vec3> &_points) {
        points = std::move(points);
    }
}