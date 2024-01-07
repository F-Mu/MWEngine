#pragma once

//std

//libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "function/global/engine_global_context.h"
#include "function/input/input_system.h"
#include <iostream>
//#include "core/math/math_headers.h"

namespace MW {
    class RenderCamera {
    public:
//        void move(glm::vec3 delta);
//
//        glm::vec3 forward() const { return (invRotation * Y); }
//
//        glm::vec3 up() const { return (invRotation * Z); }
//
//        glm::vec3 right() const { return (invRotation * X); }
//
//        Matrix4x4 getViewMatrix() const;
//
//        Matrix4x4 getPersProjMatrix() const;
//
//        void rotate(Vector2 delta);
//
//        Vector2 getFOV() const { return {fovX, fovY}; }
//
        void setAspect(float as) { aspect = as; };

        void initialize();
//
//        glm::vec3 getPosition() const { return position; }
//
//        Quaternion getRotation() const { return rotation; }
//
//        void UVWFrame(glm::vec3& U, glm::vec3& V, glm::vec3& W) const;
        float getNearClip() {
            return znear;
        }

        float getFarClip() {
            return zfar;
        }

        void setPerspective(float fov, float aspect, float znear, float zfar) {
            this->fov = fov;
            this->znear = znear;
            this->zfar = zfar;
            matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
            if (flipY) {
                matrices.perspective[1][1] *= -1.0f;
            }
        };

        void updateAspectRatio(float aspect) {
            matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
            if (flipY) {
                matrices.perspective[1][1] *= -1.0f;
            }
        }

        void setPosition(glm::vec3 position) {
            this->position = position;
            updateViewMatrix();
        }

        void setRotation(glm::vec3 rotation) {
            this->rotation = rotation;
            updateViewMatrix();
        }

        void rotate(glm::vec3 delta) {
            this->rotation += delta * rotationSpeed;
            if (rotation.x > 89.f)rotation.x = 89.f;
            if (rotation.x < -89.f)rotation.x = -89.f;
            updateViewMatrix();
        }

        void setTranslation(glm::vec3 translation) {
            this->position = translation;
            updateViewMatrix();
        };

        void translate(glm::vec3 delta) {
            this->position += delta;
            updateViewMatrix();
        }

        void setRotationSpeed(float rotationSpeed) {
            this->rotationSpeed = rotationSpeed;
        }

        void setMovementSpeed(float movementSpeed) {
            this->movementSpeed = movementSpeed;
        }

        void update(float deltaTime) {
            updated = false;
            glm::vec3 camFront;
            camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
            camFront.y = sin(glm::radians(rotation.x));
            camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
            camFront = glm::normalize(camFront);

            float moveSpeed = deltaTime * movementSpeed;

            auto keyCommand = engineGlobalContext.inputSystem->getKeyCommand();
//        Quaternion rotate = camera->getRotation().inverse();
            glm::vec3 deltaPos(0, 0, 0);

            if ((unsigned int) KeyCommand::up & keyCommand) {
                position += glm::vec3(0.0f, 1.0f, 0.0f) * moveSpeed;
            }
            if ((unsigned int) KeyCommand::down & keyCommand) {
                position -= glm::vec3(0.0f, 1.0f, 0.0f) * moveSpeed;
            }
            if ((unsigned int) KeyCommand::left & keyCommand) {
                position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
            }
            if ((unsigned int) KeyCommand::right & keyCommand) {
                position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
            }
            if ((unsigned int) KeyCommand::forward & keyCommand) {
                position += camFront * moveSpeed;
            }
            if ((unsigned int) KeyCommand::backward & keyCommand) {
                position -= camFront * moveSpeed;
            }
            updateViewMatrix();
        };
        glm::vec3 rotation = glm::vec3();
        glm::vec3 position = glm::vec3();
        glm::vec4 viewPos = glm::vec4();

        float rotationSpeed = 1.0f;
        float movementSpeed = 10.0f;

        bool updated = false;
        bool flipY = false;
        float cameraSpeed{0.05f};

        struct {
            glm::mat4 perspective;
            glm::mat4 view;
        } matrices;
//        float fovX{45.f};
//        float fovY{0.f};
        float aspect{0.f};
    private:
        float fov;
        float znear, zfar;

        void updateViewMatrix() {
            glm::mat4 rotM = glm::mat4(1.0f);
            glm::mat4 transM;

            rotM = glm::rotate(rotM, glm::radians(rotation.x * (flipY ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
            rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

            glm::vec3 translation = position;
            if (flipY) {
                translation.y *= -1.0f;
            }
            transM = glm::translate(glm::mat4(1.0f), translation);

            matrices.view = rotM * transM;

            viewPos = glm::vec4(position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

            updated = true;
        };


        void onScroll(double xscroll, double yscroll);
//        glm::vec3 position{0.0f, 0.0f, 0.0f};
//        Quaternion rotation{Quaternion::IDENTITY};
//        Quaternion invRotation{Quaternion::IDENTITY};
//        static constexpr float MIN_FOV{10.0f};
//        static constexpr float MAX_FOV{89.0f};
//        static const glm::vec3 X, Y, Z;

        // for ray tracing
//        glm::vec3 lookAt{10.0f, 0.0f, 10.0f};
//        glm::vec3 lookUp{0.0f, 1.0f, 0.0f};
    };

//    inline const glm::vec3 RenderCamera::X = {1.0f, 0.0f, 0.0f};
//    inline const glm::vec3 RenderCamera::Y = {0.0f, 0.0f, 1.0f};
//    inline const glm::vec3 RenderCamera::Z = {0.0f, 1.0f, 0.0f};
}