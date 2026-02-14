#pragma once

#include "math/camera.h"

namespace core
{
    class CameraController
    {
    public:
        void ResetFromCamera(const math::Camera& camera);
        void OnMouseMoveNdc(float ndcX, float ndcY);
        void Update(float dtSeconds, math::Camera& camera);

    private:
        float yaw_ = 0.0f;
        float pitch_ = 0.0f;
        float moveSpeed_ = 2.5f;
        float mouseLookSensitivity_ = 2.2f;
        float prevMouseNdcX_ = 0.0f;
        float prevMouseNdcY_ = 0.0f;
        bool hasPrevMouse_ = false;
    };
}
