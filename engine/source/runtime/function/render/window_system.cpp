#include "window_system.h"

#include <stdexcept>

namespace MW {
    WindowSystem::~WindowSystem() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void WindowSystem::initialize(WindowCreateInfo& create_info) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        width = create_info.width;
        height = create_info.height;
        window = glfwCreateWindow(create_info.width, create_info.height, create_info.title, nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetCharCallback(window, charCallback);
        glfwSetCharModsCallback(window, charModsCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);
        glfwSetCursorEnterCallback(window, cursorEnterCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetDropCallback(window, dropCallback);
        glfwSetWindowSizeCallback(window, windowSizeCallback);
        glfwSetWindowCloseCallback(window, windowCloseCallback);

        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    }
    void WindowSystem::setTitle(const char* title) { glfwSetWindowTitle(window, title); }

}