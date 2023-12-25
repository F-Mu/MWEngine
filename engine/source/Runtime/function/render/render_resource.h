#pragma once

#include "core/math/math_headers.h"
#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>
#include <map>
#include <vector>
#include <cmath>

namespace MW {
    // TODO:给proj_view_matrix
    struct CameraObject {
        Matrix4x4 viewMatrix;
        Matrix4x4 projMatrix;
    };

    class RenderResource {
    public:
        CameraObject cameraObject;
    };
} // namespace MW
