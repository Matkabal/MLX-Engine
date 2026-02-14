#pragma once

#include <cstdint>

namespace core
{
    class InputController
    {
    public:
        void SetViewport(uint32_t width, uint32_t height);
        void OnMouseMovePixels(int32_t x, int32_t y);
        void OnLeftMouseButton(bool isDown);

        float GetMouseNdcX() const { return mouseNdcX_; }
        float GetMouseNdcY() const { return mouseNdcY_; }
        bool IsLeftMouseDown() const { return leftMouseDown_; }

    private:
        uint32_t viewportWidth_ = 1;
        uint32_t viewportHeight_ = 1;
        float mouseNdcX_ = 0.0f;
        float mouseNdcY_ = 0.0f;
        bool leftMouseDown_ = false;
    };
}
