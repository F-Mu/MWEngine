#include "game_object.h"
#include "render/render_device.h"
#include "core/macro.h"
#include <iostream>

namespace MW {
    void GameObject::tick() {
        if (!shouldTick)return;
        for (auto &component: components) {
            component->tick();
        }
    }

    GameObject::~GameObject() {
        components.clear();
    }
}