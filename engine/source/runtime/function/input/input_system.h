#pragma once

#include "runtime/core/math/math.h"
#include "glm/glm.hpp"
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

        void onMouseButton(int button, int action, int mods);

        void initialize();

        void tick();

        uint32_t getKeyCommand() { return keyCommand; }
//        void processKeyCommand();
        struct MouseState{
            struct {
                bool left = false;
                bool right = false;
                bool middle = false;
            } buttons;
            glm::vec2 position;
        };
        void clear();
        const MouseState& getMouseState(){return mouseState;}
    private:
        /** @brief State of mouse/touch input */
        MouseState mouseState;
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