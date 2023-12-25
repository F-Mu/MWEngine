#include "engine.h"
#include "function/global/engine_global_context.h"
#include "function/render/render_system.h"
#include "function/input/input_system.h"

namespace MW {
    void MWEngine::run() {
    }

    void MWEngine::logicalTick(float deltaTime) {
        engineGlobalContext.inputSystem->tick();
    }

    void MWEngine::tick(float deltaTime) {
        logicalTick(deltaTime);
        engineGlobalContext.renderSystem->tick(deltaTime);
    }

    void MWEngine::initialize() {
        engineGlobalContext.initialize();
    }

    void MWEngine::clear() {
        engineGlobalContext.clear();
    }
}
