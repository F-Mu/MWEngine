#pragma once
#define GLFW_INCLUDE_VULKAN

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <string>
#include <array>
#include <functional>

namespace MW {
    struct WindowCreateInfo {
        int width{800};
        int height{600};
        const char *title{"MWEngine"};
        bool isFullscreen{false};
    };

    class WindowSystem {
    public:

        WindowSystem() = default;

        ~WindowSystem();

        WindowSystem(const WindowSystem &) = delete;

        WindowSystem &operator=(const WindowSystem &) = delete;

        bool shouldClose() { return glfwWindowShouldClose(window); }

        std::array<int, 2> getWindowSize() { return std::array<int, 2>({width, height}); }

        GLFWwindow *getGLFWwindow() const { return window; }

        void initialize(WindowCreateInfo &create_info);

        typedef std::function<void()> onResetFunc;
        typedef std::function<void(int, int, int, int)> onKeyFunc;
        typedef std::function<void(unsigned int)> onCharFunc;
        typedef std::function<void(int, unsigned int)> onCharModsFunc;
        typedef std::function<void(int, int, int)> onMouseButtonFunc;
        typedef std::function<void(double, double)> onCursorPosFunc;
        typedef std::function<void(int)> onCursorEnterFunc;
        typedef std::function<void(double, double)> onScrollFunc;
        typedef std::function<void(int, const char **)> onDropFunc;
        typedef std::function<void(int, int)> onWindowSizeFunc;
        typedef std::function<void()> onWindowCloseFunc;

        void registerOnResetFunc(onResetFunc func) { ResetFunc.push_back(func); }

        void registerOnKeyFunc(onKeyFunc func) { KeyFunc.push_back(func); }

        void registerOnCharFunc(onCharFunc func) { CharFunc.push_back(func); }

        void registerOnCharModsFunc(onCharModsFunc func) { CharModsFunc.push_back(func); }

        void registerOnMouseButtonFunc(onMouseButtonFunc func) { MouseButtonFunc.push_back(func); }

        void registerOnCursorPosFunc(onCursorPosFunc func) { CursorPosFunc.push_back(func); }

        void registerOnCursorEnterFunc(onCursorEnterFunc func) { CursorEnterFunc.push_back(func); }

        void registerOnScrollFunc(onScrollFunc func) { ScrollFunc.push_back(func); }

        void registerOnDropFunc(onDropFunc func) { DropFunc.push_back(func); }

        void registerOnWindowSizeFunc(onWindowSizeFunc func) { WindowSizeFunc.push_back(func); }

        void registerOnWindowCloseFunc(onWindowCloseFunc func) { WindowCloseFunc.push_back(func); }

        bool isMouseButtonDown(int button) const {
            if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST) {
                return false;
            }
            return glfwGetMouseButton(window, button) == GLFW_PRESS;
        }

    protected:
        // window event callbacks
        static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
            WindowSystem *app = (WindowSystem *) glfwGetWindowUserPointer(window);
            if (app) {
                app->onKey(key, scancode, action, mods);
            }
        }

        static void charCallback(GLFWwindow *window, unsigned int codepoint) {
            WindowSystem *app = (WindowSystem *) glfwGetWindowUserPointer(window);
            if (app) {
                app->onChar(codepoint);
            }
        }

        static void charModsCallback(GLFWwindow *window, unsigned int codepoint, int mods) {
            WindowSystem *app = (WindowSystem *) glfwGetWindowUserPointer(window);
            if (app) {
                app->onCharMods(codepoint, mods);
            }
        }

        static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
            WindowSystem *app = (WindowSystem *) glfwGetWindowUserPointer(window);
            if (app) {
                app->onMouseButton(button, action, mods);
            }
        }

        static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
            WindowSystem *app = (WindowSystem *) glfwGetWindowUserPointer(window);
            if (app) {
                app->onCursorPos(xpos, ypos);
            }
        }

        static void cursorEnterCallback(GLFWwindow *window, int entered) {
            WindowSystem *app = (WindowSystem *) glfwGetWindowUserPointer(window);
            if (app) {
                app->onCursorEnter(entered);
            }
        }

        static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
            WindowSystem *app = (WindowSystem *) glfwGetWindowUserPointer(window);
            if (app) {
                app->onScroll(xoffset, yoffset);
            }
        }

        static void dropCallback(GLFWwindow *window, int count, const char **paths) {
            WindowSystem *app = (WindowSystem *) glfwGetWindowUserPointer(window);
            if (app) {
                app->onDrop(count, paths);
            }
        }

        static void windowSizeCallback(GLFWwindow *window, int width, int height) {
            WindowSystem *app = (WindowSystem *) glfwGetWindowUserPointer(window);
            if (app) {
                app->width = width;
                app->height = height;
            }
        }

        static void windowCloseCallback(GLFWwindow *window) { glfwSetWindowShouldClose(window, true); }

        void onReset() {
            for (auto &func: ResetFunc)
                func();
        }

        void onKey(int key, int scancode, int action, int mods) {
            for (auto &func: KeyFunc)
                func(key, scancode, action, mods);
        }

        void onChar(unsigned int codepoint) {
            for (auto &func: CharFunc)
                func(codepoint);
        }

        void onCharMods(int codepoint, unsigned int mods) {
            for (auto &func: CharModsFunc)
                func(codepoint, mods);
        }

        void onMouseButton(int button, int action, int mods) {
            for (auto &func: MouseButtonFunc)
                func(button, action, mods);
        }

        void onCursorPos(double xpos, double ypos) {
            for (auto &func: CursorPosFunc)
                func(xpos, ypos);
        }

        void onCursorEnter(int entered) {
            for (auto &func: CursorEnterFunc)
                func(entered);
        }

        void onScroll(double xoffset, double yoffset) {
            for (auto &func: ScrollFunc)
                func(xoffset, yoffset);
        }

        void onDrop(int count, const char **paths) {
            for (auto &func: DropFunc)
                func(count, paths);
        }

        void onWindowSize(int width, int height) {
            for (auto &func: WindowSizeFunc)
                func(width, height);
        }

    private:
        int width;
        int height;

        GLFWwindow *window;


        std::vector<onResetFunc> ResetFunc;
        std::vector<onKeyFunc> KeyFunc;
        std::vector<onCharFunc> CharFunc;
        std::vector<onCharModsFunc> CharModsFunc;
        std::vector<onMouseButtonFunc> MouseButtonFunc;
        std::vector<onCursorPosFunc> CursorPosFunc;
        std::vector<onCursorEnterFunc> CursorEnterFunc;
        std::vector<onScrollFunc> ScrollFunc;
        std::vector<onDropFunc> DropFunc;
        std::vector<onWindowSizeFunc> WindowSizeFunc;
        std::vector<onWindowCloseFunc> WindowCloseFunc;

    };
}