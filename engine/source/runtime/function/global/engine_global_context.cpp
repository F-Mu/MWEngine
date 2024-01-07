#include "engine_global_context.h"

#include "function/render/window_system.h"
#include "function/render/render_system.h"
#include "function/render/scene_manager.h"
#include "function/input/input_system.h"

namespace MW {
    EngineGlobalContext engineGlobalContext;

    void EngineGlobalContext::initialize() {
        windowSystem = std::make_shared<WindowSystem>();
        WindowCreateInfo windowInfo;
        windowSystem->initialize(windowInfo);
        RenderSystemInitInfo renderInfo;
        renderInfo.window = windowSystem;
        renderSystem = std::make_shared<RenderSystem>();
        renderSystem->initialize(renderInfo);
        inputSystem = std::make_shared<InputSystem>();
        inputSystem->initialize();
    }

    void EngineGlobalContext::clear() {
        inputSystem.reset();
        renderSystem.reset();
        windowSystem.reset();
    }

    std::shared_ptr<SceneManager> EngineGlobalContext::getScene() {
        return renderSystem->getScene();
    }
}