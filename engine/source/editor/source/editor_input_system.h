#pragma once

#include "runtime/core/math/math.h"

namespace MW {
    enum class KeyCommand : unsigned int {
        forward = 1 << 0,               // W
        backward = 1 << 1,              // S
        left = 1 << 2,                  // A
        right = 1 << 3,                 // D
        up = 1 << 4,                    // Q
        down = 1 << 5,                  // W
    };

    class InputSystem {
    public:
        void onKey(int key, int scancode, int action, int mods);
    private:
        uint32_t keyCommand;
    };
}