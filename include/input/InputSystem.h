#pragma once

#include "math/vec2.h"

namespace input
{
    struct InputFrame
    {
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;

        bool leftMouseDown = false;
        bool rightMouseDown = false;
        math::Vec2 mouseNdc{};
        math::Vec2 mouseDeltaNdc{};
        float wheelDelta = 0.0f;
    };

    class InputSystem
    {
    public:
        void SetMouseNdc(float x, float y);
        void SetLeftMouse(bool down);
        void SetRightMouse(bool down);
        void AddWheelDelta(float delta);
        InputFrame BuildFrameInput();

    private:
        math::Vec2 mouseNdc_{};
        math::Vec2 mouseDeltaNdc_{};
        bool leftMouseDown_ = false;
        bool rightMouseDown_ = false;
        float wheelDelta_ = 0.0f;
    };
}

