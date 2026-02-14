#include "core/InputController.h"
#include "core/Logger.h"

namespace core
{
    void InputController::SetViewport(uint32_t width, uint32_t height)
    {
        LOG_METHOD("InputController", "SetViewport");
        viewportWidth_ = width == 0 ? 1 : width;
        viewportHeight_ = height == 0 ? 1 : height;
    }

    void InputController::OnMouseMovePixels(int32_t x, int32_t y)
    {
        LOG_METHOD("InputController", "OnMouseMovePixels");
        const float fx = static_cast<float>(x);
        const float fy = static_cast<float>(y);
        const float w = static_cast<float>(viewportWidth_);
        const float h = static_cast<float>(viewportHeight_);

        mouseNdcX_ = (fx / w) * 2.0f - 1.0f;
        mouseNdcY_ = 1.0f - (fy / h) * 2.0f;
    }

    void InputController::OnLeftMouseButton(bool isDown)
    {
        LOG_METHOD("InputController", "OnLeftMouseButton");
        leftMouseDown_ = isDown;
    }
}
