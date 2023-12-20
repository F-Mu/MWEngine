#include "geometry.h"

namespace MW {
    std::vector<Vertex>
    Geometry::fillMesh(const std::vector<glm::vec3> &points, const glm::vec3 &color) {
        std::vector<Vertex> vertices;
        for (auto point: points) {
            vertices.emplace_back(point, color);
        }
        return vertices;
    }

    std::vector<Vertex>
    Geometry::unfilledMesh(std::vector<glm::vec3> &points, const glm::vec3 &color) {
        std::vector<Vertex> vertices;
        for (int i = 0; i + 1 < points.size(); ++i) {
            auto tmp = line(points[i], points[i + 1], color);
            vertices.insert(vertices.end(), tmp.begin(), tmp.end());
        }
        auto tmp = line(points.back(), points[0], color);
        vertices.insert(vertices.end(), tmp.begin(), tmp.end());
        return vertices;
    }

    std::vector<Vertex>
    Geometry::line(glm::vec3 x, glm::vec3 y, const glm::vec3 &color) {
        //向线的法向方向平移从而画出带厚度的线
        auto tt = y - x;
        tt = {-tt.y, tt.x, tt.z};
        glm::vec3 off = offset * glm::normalize(tt);
        auto line = std::move(fillMesh(
                {
                        x - off, x + off, y - off, y + off
                }, color));
        return line;
    }
}