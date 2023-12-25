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

    struct CameraUVWObject {
        Vector3 camera_position;
        float _padding_camera_pos;
        Vector3 camera_U;
        float _padding_camera_U;
        Vector3 camera_V;
        float _padding_camera_V;
        Vector3 camera_W;
        float _padding_camera_W;
    };

    class RenderResource {
    public:
        CameraObject cameraObject;
        CameraUVWObject cameraUVWObject;
    };
} // namespace MW
