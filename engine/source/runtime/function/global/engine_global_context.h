#pragma once

#include <memory>

namespace MW {
    class MWEngine;

    class RenderSystem;

    class WindowSystem;

    class InputSystem;

    class SceneManager;

    class EngineGlobalContext {
    public:
        void initialize();

        void clear();

    public:

        std::shared_ptr<WindowSystem> windowSystem;
        std::shared_ptr<RenderSystem> renderSystem;
        std::shared_ptr<InputSystem> inputSystem;
        std::shared_ptr<SceneManager> sceneManager;
    };

    extern EngineGlobalContext engineGlobalContext;
}