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

        void onCursorPos(double currentCursorX, double currentCursorY);

        void initialize();

        void tick();

        uint32_t getKeyCommand() { return keyCommand; }
//        void processKeyCommand();

        void clear();

    private:
        uint32_t keyCommand;

        void calculateCursorDeltaAngles();

        int cursorDeltaX{0};
        int cursorDeltaY{0};

        Radian yaw{0};
        Radian pitch{0};
        int lastCursorX{0};
        int lastCursorY{0};
    };
}