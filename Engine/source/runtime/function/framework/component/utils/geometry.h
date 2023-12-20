#pragma once

#include "core/vertex.h"

namespace MW {
    class Geometry {
    public:
        static std::vector<Vertex>
        fillMesh(const std::vector<glm::vec3> &points, const glm::vec3 &color);

        static std::vector<Vertex>
        unfilledMesh(std::vector<glm::vec3> &points, const glm::vec3 &color);

        static std::vector<Vertex>
        line(glm::vec3 x, glm::vec3 y, const glm::vec3 &color);

    private:
        constexpr static float offset = 0.0025;
    };
}