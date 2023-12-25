#pragma once

//std

//libs
#define GLFORCE_RADIANS
#define GLFORCE_DEPTH_ZERO_TO_ONE

#include "core/math/math_headers.h"

namespace MW {
    class RenderCamera { //根据Piccolo引擎代码，是根据控制Aspect和fovX来控制摄像机，fovY通过计算获得
    public:
        void move(Vector3 delta);

        Vector3 forward() const { return (invRotation * Y); }

        Vector3 up() const { return (invRotation * Z); }

        Vector3 right() const { return (invRotation * X); }

        Matrix4x4 getViewMatrix() const;

        Matrix4x4 getPersProjMatrix() const;

        void rotate(Vector2 delta);

        Vector2 getFOV() const { return {fovX, fovY}; }

        void setAspect(float as);

        Vector3 getPosition() const { return position; }

        Quaternion getRotation() const { return rotation; }

        void UVWFrame(Vector3& U, Vector3& V, Vector3& W) const;

    private:
        float fovX{Degree(89.f).valueDegrees()};
        float fovY{0.f};
        float near{1000.0f};
        float far{0.1f};
        float aspect{0.f};
        Vector3 position{0.0f, 0.0f, 0.0f};
        Quaternion rotation{Quaternion::IDENTITY};
        Quaternion invRotation{Quaternion::IDENTITY};
        static constexpr float MIN_FOV{10.0f};
        static constexpr float MAX_FOV{89.0f};
        static const Vector3 X, Y, Z;

        // for ray tracing
        Vector3 lookAt{0.0f, 0.0f, 1.0f};
        Vector3 lookUp{0.0f, 1.0f, 0.0f};
    };

    inline const Vector3 RenderCamera::X = {1.0f, 0.0f, 0.0f};
    inline const Vector3 RenderCamera::Y = {0.0f, 1.0f, 0.0f};
    inline const Vector3 RenderCamera::Z = {0.0f, 0.0f, 1.0f};
}