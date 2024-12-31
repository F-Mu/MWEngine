#include "input_system.h"
#include "runtime/function/global/engine_global_context.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/render_camera.h"
#include <array>

namespace MW {

    constexpr uint32_t commandMask = 0xFFFFFFFF;

    void InputSystem::onKey(int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            switch (key) {
                case GLFW_KEY_A:
                    keyCommand |= (unsigned int) KeyCommand::left;
                    break;
                case GLFW_KEY_S:
                    keyCommand |= (unsigned int) KeyCommand::backward;
                    break;
                case GLFW_KEY_W:
                    keyCommand |= (unsigned int) KeyCommand::forward;
                    break;
                case GLFW_KEY_D:
                    keyCommand |= (unsigned int) KeyCommand::right;
                    break;
                case GLFW_KEY_Q:
                    keyCommand |= (unsigned int) KeyCommand::up;
                    break;
                case GLFW_KEY_E:
                    keyCommand |= (unsigned int) KeyCommand::down;
                    break;
                default:
                    break;
            }
        } else if (action == GLFW_RELEASE) {
            switch (key) {
                case GLFW_KEY_A:
                    keyCommand &= (commandMask ^ (unsigned int) KeyCommand::left);
                    break;
                case GLFW_KEY_S:
                    keyCommand &= (commandMask ^ (unsigned int) KeyCommand::backward);
                    break;
                case GLFW_KEY_W:
                    keyCommand &= (commandMask ^ (unsigned int) KeyCommand::forward);
                    break;
                case GLFW_KEY_D:
                    keyCommand &= (commandMask ^ (unsigned int) KeyCommand::right);
                    break;
                case GLFW_KEY_Q:
                    keyCommand &= (commandMask ^ (unsigned int) KeyCommand::up);
                    break;
                case GLFW_KEY_E:
                    keyCommand &= (commandMask ^ (unsigned int) KeyCommand::down);
                    break;
                default:
                    break;
            }
        }
    }

    void InputSystem::onCursorPos(double currentCursorX, double currentCursorY) {
        cursorDeltaX = lastCursorX - currentCursorX; //为什么之前减现在
        cursorDeltaY = lastCursorY - currentCursorY;
        std::array<int, 2> windowSize = engineGlobalContext.windowSystem->getWindowSize();
        float angularVelocity =
                180.0f / Math::max(windowSize[0], windowSize[1]); // 180 degrees while moving full screen
        if (engineGlobalContext.windowSystem->isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
            engineGlobalContext.renderSystem->getRenderCamera()->rotate(
                    glm::vec3(cursorDeltaY, -cursorDeltaX, 0.0f));
        }
        lastCursorX = currentCursorX;
        lastCursorY = currentCursorY;
    }

    void InputSystem::onMouseButton(int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (engineGlobalContext.windowSystem->isMouseButtonDown(button)) {
                mouseState.buttons.left = true;
                mouseState.position = {lastCursorX, lastCursorY};
            } else {
                mouseState.buttons.left = false;
            }
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (engineGlobalContext.windowSystem->isMouseButtonDown(button)) {
                mouseState.buttons.right = true;
                mouseState.position = {lastCursorX, lastCursorY};
            } else {
                mouseState.buttons.right = false;
            }
        }
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (engineGlobalContext.windowSystem->isMouseButtonDown(button)) {
                mouseState.buttons.middle = true;
                mouseState.position = {lastCursorX, lastCursorY};
            } else {
                mouseState.buttons.middle = false;
            }
        }
    }

    void InputSystem::initialize() {

        std::shared_ptr<WindowSystem> windowSystem = engineGlobalContext.windowSystem;

        windowSystem->registerOnKeyFunc(std::bind(&InputSystem::onKey,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3,
                                                  std::placeholders::_4));
        windowSystem->registerOnCursorPosFunc(
                std::bind(&InputSystem::onCursorPos, this, std::placeholders::_1, std::placeholders::_2));
        windowSystem->registerOnMouseButtonFunc(std::bind(&InputSystem::onMouseButton, this,
                                                          std::placeholders::_1,
                                                          std::placeholders::_2,
                                                          std::placeholders::_3));
    }

    void InputSystem::tick() {
//        calculateCursorDeltaAngles();
//        clear();
    }

    void InputSystem::clear() {
        cursorDeltaX = 0;
        cursorDeltaY = 0;
    }

    void InputSystem::calculateCursorDeltaAngles() {
//        std::array<int, 2> windowSize = engineGlobalContext.windowSystem->getWindowSize();
//
//        if (windowSize[0] < 1 || windowSize[1] < 1) {
//            return;
//        }
//
//        std::shared_ptr<RenderCamera> renderCamera = engineGlobalContext.renderSystem->getRenderCamera();
//        const Vector2 &fov = renderCamera->getFOV();
//
//        Radian deltaX(Math::degreesToRadians(cursorDeltaX));
//        Radian deltaY(Math::degreesToRadians(cursorDeltaY));
//
//        yaw = (deltaX / (float) windowSize[0]) * fov.x;
//        pitch = -(deltaY / (float) windowSize[1]) * fov.y;
    }

//    void InputSystem::processKeyCommand() {
//    }
}