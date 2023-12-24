#pragma once
#define GLFW_INCLUDE_VULKAN

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <string>
namespace MW {
    struct WindowCreateInfo {
        int width{ 800 };
        int height{ 600 };
        const char* title{ "MWEngine" };
        bool isFullscreen{ false };
    };

    class WindowSystem {
    public:

        WindowSystem() = default;

        ~WindowSystem();

        WindowSystem(const WindowSystem&) = delete;

        WindowSystem& operator=(const WindowSystem&) = delete;

        bool shouldClose() { return glfwWindowShouldClose(window); }

        VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }

        bool wasWindowResized() { return framebufferResized; }

        void resetWindowResizedFlag() { framebufferResized = false; }

        GLFWwindow* getGLFWwindow() const { return window; }

        void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

        void initialize(WindowCreateInfo& create_info);

    private:
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

        int width;
        int height;
        bool framebufferResized = false;

        GLFWwindow* window;
    };
}