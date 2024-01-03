#include "render_camera.h"
#include "function/global/engine_global_context.h"
#include "function/render/window_system.h"
//std
#include <cassert>
#include <limits>
#include <iostream>eeee

namespace MW {

    void RenderCamera::onScroll(double xscroll, double yscroll) {
        glm::vec3 camFront;
        camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
        camFront.y = sin(glm::radians(rotation.x));
        camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
        camFront = glm::normalize(camFront);
        int dir = yscroll > 0 ? 1 : -1;
        float moveSpeed = movementSpeed * dir;
        position += camFront * moveSpeed;
    }

    void RenderCamera::initialize() {
        std::shared_ptr<WindowSystem> windowSystem = engineGlobalContext.windowSystem;

        windowSystem->registerOnScrollFunc(
                std::bind(&RenderCamera::onScroll, this, std::placeholders::_1, std::placeholders::_2));
    }
//    void RenderCamera::move(Vector3 delta) {
//        Vector3 forward = (lookAt - position).normalisedCopy();
//        Vector3 up = Z;
//        Vector3 left = forward.crossProduct(up);
//        Vector3 dir = left * delta.x + up * delta.y + forward * delta.z;
//        position += dir;
//        lookAt += dir;
//    }
//
//    Matrix4x4 RenderCamera::getViewMatrix() const {
//        return Math::makeLookAtMatrix(position, position + forward(), up());
//    }
//
//    Matrix4x4 RenderCamera::getPersProjMatrix() const {
//        Matrix4x4 fix_mat(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
//        Matrix4x4 proj_mat = fix_mat * Math::makePerspectiveMatrix(Radian(Degree(fovY)), aspect, near, far);
//        return proj_mat;
////        return Math::makePerspectiveMatrix(Radian(Degree(fovY)), aspect, near, far);
//    }
//
//    void RenderCamera::rotate(Vector2 delta) {
//        // rotation around x, y axis
//        delta = Vector2(Radian(Degree(delta.x)).valueRadians(), Radian(Degree(delta.y)).valueRadians());
//        // limit pitch
//        float dot = Z.dotProduct(forward());
//        if ((dot < -0.99f && delta.x > 0.0f) || // angle nearing 180 degrees
//            (dot > 0.99f && delta.x < 0.0f))    // angle nearing 0 degrees
//            delta.x = 0.0f;
//        // pitch is relative to current sideways rotation
//        // yaw happens independently
//        // this prevents roll
//        Quaternion pitch, yaw;
//        pitch.fromAngleAxis(Radian(delta.x), X);
//        yaw.fromAngleAxis(Radian(delta.y), Z);
//        rotation = pitch * rotation * yaw;
//        invRotation = rotation.conjugate();
//        lookAt = rotation * lookAt ;
//    }
//
//    void RenderCamera::setAspect(float as) {
//        aspect = as;
//
//        // 1 / tan(fovy * 0.5) / aspect = 1 / tan(fovx * 0.5)
//        // 1 / tan(fovy * 0.5) = aspect / tan(fovx * 0.5)
//        // tan(fovy * 0.5) = tan(fovx * 0.5) / aspect
//
//        fovY = Radian(Math::atan(Math::tan(Radian(Degree(fovX) * 0.5f)) / aspect) * 2.0f).valueDegrees();
//    }
//
//    // from Optix
//    void RenderCamera::UVWFrame(Vector3 &U, Vector3 &V, Vector3 &W) const {
//        W = lookAt - position; // Do not normalize W -- it implies focal length
//        float wlen = W.length();
//        U = W.crossProduct(lookUp).normalisedCopy();
//        V = U.crossProduct(W).normalisedCopy();
//
//        float vlen = wlen * tanf(0.5f * fovY * Math_PI / 180.0f);
//        V *= vlen;
//        float ulen = vlen * aspect;
//        U *= ulen;
//    }
}