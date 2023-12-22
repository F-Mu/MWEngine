#include "game_object.h"

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