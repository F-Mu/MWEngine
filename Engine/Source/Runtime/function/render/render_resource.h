#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <vector>
#include <cmath>

namespace MW {
    // TODO:给proj_view_matrix
    struct CameraObject {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
    };

    class RenderResource {
    public:
        CameraObject cameraObject;
    };
} // namespace MW
