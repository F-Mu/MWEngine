#include "engine.h"
#include "function/global/engine_global_context.h"
#include "function/render/render_system.h"
#include "function/input/input_system.h"
#include "function/render/window_system.h"

namespace MW {

    const float MWEngine::fpsAlpha = 1.f / 100;

    void MWEngine::run() {
    }

    void MWEngine::logicalTick(float deltaTime) {
        engineGlobalContext.inputSystem->tick();
    }

    void MWEngine::calculateFPS(float delta_time) {
        frameCount++;

        if (frameCount == 1) {
            averageDuration = delta_time;
        } else {
            averageDuration = averageDuration * (1 - fpsAlpha) + delta_time * fpsAlpha;
        }

        fps = static_cast<int>(1.f / averageDuration);
    }

    void MWEngine::tick(float deltaTime) {
        logicalTick(deltaTime);
        calculateFPS(deltaTime);
        engineGlobalContext.renderSystem->tick(deltaTime);

        engineGlobalContext.windowSystem->setTitle(
                std::string("MWEngine - " + std::to_string(fps) + " FPS").c_str());
    }

    void MWEngine::initialize() {
        engineGlobalContext.initialize();
    }

    void MWEngine::clear() {
        engineGlobalContext.clear();
    }
}
