#include "core/CameraController.h"

#include <algorithm>
#include <cmath>
#include <windows.h>

#include "core/Logger.h"
#include "math/vec3.h"

namespace core
{
    void CameraController::ResetFromCamera(const math::Camera& camera)
    {
        LOG_METHOD("CameraController", "ResetFromCamera");
        const math::Vec3 eye = camera.GetPosition();
        math::Vec3 forward = math::Normalize(camera.GetTarget() - eye);
        if (math::Length(forward) <= 1e-5f)
        {
            forward = math::Vec3{0.0f, 0.0f, 1.0f};
        }

        pitch_ = std::asin(forward.y);
        yaw_ = std::atan2(forward.x, forward.z);
        hasPrevMouse_ = false;
    }

    void CameraController::OnMouseMoveNdc(float ndcX, float ndcY)
    {
        LOG_METHOD("CameraController", "OnMouseMoveNdc");
        if (hasPrevMouse_ && (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0)
        {
            const float dx = ndcX - prevMouseNdcX_;
            const float dy = ndcY - prevMouseNdcY_;
            yaw_ += dx * mouseLookSensitivity_;
            pitch_ += dy * mouseLookSensitivity_;
        }
        prevMouseNdcX_ = ndcX;
        prevMouseNdcY_ = ndcY;
        hasPrevMouse_ = true;
    }

    void CameraController::Update(float dtSeconds, math::Camera& camera)
    {
        LOG_METHOD("CameraController", "Update");
        const float maxPitch = 1.48352986f;
        pitch_ = std::clamp(pitch_, -maxPitch, maxPitch);

        const float cosPitch = std::cos(pitch_);
        math::Vec3 forward{
            std::sin(yaw_) * cosPitch,
            std::sin(pitch_),
            std::cos(yaw_) * cosPitch,
        };
        forward = math::Normalize(forward);

        math::Vec3 eye = camera.GetPosition();
        if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0)
        {
            const math::Vec3 worldUp{0.0f, 1.0f, 0.0f};
            math::Vec3 right = math::Normalize(math::Cross(worldUp, forward));
            if (math::Length(right) <= 1e-5f)
            {
                right = math::Vec3{1.0f, 0.0f, 0.0f};
            }

            float speed = moveSpeed_;
            if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
            {
                speed *= 2.5f;
            }
            const float step = speed * dtSeconds;

            if ((GetAsyncKeyState('W') & 0x8000) != 0)
            {
                eye = eye + (forward * step);
            }
            if ((GetAsyncKeyState('S') & 0x8000) != 0)
            {
                eye = eye - (forward * step);
            }
            if ((GetAsyncKeyState('A') & 0x8000) != 0)
            {
                eye = eye - (right * step);
            }
            if ((GetAsyncKeyState('D') & 0x8000) != 0)
            {
                eye = eye + (right * step);
            }
            if ((GetAsyncKeyState('Q') & 0x8000) != 0)
            {
                eye.y -= step;
            }
            if ((GetAsyncKeyState('E') & 0x8000) != 0)
            {
                eye.y += step;
            }

            camera.SetPosition(eye);
        }

        camera.SetTarget(eye + forward);
    }
}
