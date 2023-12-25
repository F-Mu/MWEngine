#include "render_camera.h"
#include "function/global/engine_global_context.h"
//std
#include <cassert>
#include <limits>

namespace MW {
    void RenderCamera::move(Vector3 delta) { position += delta; }

    Matrix4x4 RenderCamera::getViewMatrix() const {
        return Math::makeLookAtMatrix(position, position + forward(), up());
    }

    Matrix4x4 RenderCamera::getPersProjMatrix() const {
        Matrix4x4 fix_mat(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        Matrix4x4 proj_mat = fix_mat * Math::makePerspectiveMatrix(Radian(Degree(fovY)), aspect, near, far);
        return proj_mat;
//        return Math::makePerspectiveMatrix(Radian(Degree(fovY)), aspect, near, far);
    }

    void RenderCamera::rotate(Vector2 delta) {
        // rotation around x, y axis
        delta = Vector2(Radian(Degree(delta.x)).valueRadians(), Radian(Degree(delta.y)).valueRadians());
        // limit pitch
        float dot = Z.dotProduct(forward());
        if ((dot < -0.99f && delta.x > 0.0f) || // angle nearing 180 degrees
            (dot > 0.99f && delta.x < 0.0f))    // angle nearing 0 degrees
            delta.x = 0.0f;
        // pitch is relative to current sideways rotation
        // yaw happens independently
        // this prevents roll
        Quaternion pitch, yaw;
        pitch.fromAngleAxis(Radian(delta.x), X);
        yaw.fromAngleAxis(Radian(delta.y), Z);
        rotation = pitch * rotation * yaw;
        invRotation = rotation.conjugate();
    }

    void RenderCamera::setAspect(float as) {
        aspect = as;

        // 1 / tan(fovy * 0.5) / aspect = 1 / tan(fovx * 0.5)
        // 1 / tan(fovy * 0.5) = aspect / tan(fovx * 0.5)
        // tan(fovy * 0.5) = tan(fovx * 0.5) / aspect

        fovY = Radian(Math::atan(Math::tan(Radian(Degree(fovX) * 0.5f)) / aspect) * 2.0f).valueDegrees();
    }
}