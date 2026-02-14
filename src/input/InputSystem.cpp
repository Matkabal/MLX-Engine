#include "input/InputSystem.h"

#include <windows.h>

namespace input
{
    void InputSystem::SetMouseNdc(float x, float y)
    {
        mouseDeltaNdc_ = math::Vec2{x - mouseNdc_.x, y - mouseNdc_.y};
        mouseNdc_ = math::Vec2{x, y};
    }

    void InputSystem::SetLeftMouse(bool down)
    {
        leftMouseDown_ = down;
    }

    void InputSystem::SetRightMouse(bool down)
    {
        rightMouseDown_ = down;
    }

    void InputSystem::AddWheelDelta(float delta)
    {
        wheelDelta_ += delta;
    }

    InputFrame InputSystem::BuildFrameInput()
    {
        InputFrame frame{};
        frame.up = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
        frame.down = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
        frame.left = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
        frame.right = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
        frame.leftMouseDown = leftMouseDown_;
        frame.rightMouseDown = ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) || rightMouseDown_;
        frame.mouseNdc = mouseNdc_;
        frame.mouseDeltaNdc = mouseDeltaNdc_;
        frame.wheelDelta = wheelDelta_;

        mouseDeltaNdc_ = math::Vec2{};
        wheelDelta_ = 0.0f;
        return frame;
    }
}
