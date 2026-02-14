#include "editor/SceneCamera.h"

#include <algorithm>
#include <cmath>

namespace editor
{
    void SceneCamera::Update(const SceneCameraInput& input, float dtSeconds)
    {
        const float dt = (dtSeconds > 0.0f) ? dtSeconds : (1.0f / 60.0f);
        const float blend = std::clamp(smoothing_ * dt, 0.0f, 1.0f);

        if (input.altDown && input.leftDown)
        {
            yaw_ += input.mouseDelta.x * orbitSpeed_ * dt;
            pitch_ += input.mouseDelta.y * orbitSpeed_ * dt;
            pitch_ = std::clamp(pitch_, -1.45f, 1.45f);
        }

        if (input.altDown && input.rightDown)
        {
            distance_ -= input.mouseDelta.y * zoomSpeed_ * dt;
        }
        distance_ -= input.wheelDelta * zoomSpeed_ * dt;
        distance_ = std::clamp(distance_, 0.3f, 200.0f);

        math::Vec3 forward{};
        math::Vec3 right{};
        math::Vec3 up{};
        UpdateBasis(forward, right, up);

        if (input.altDown && input.middleDown)
        {
            const float panFactor = std::max(0.2f, distance_ * 0.1f) * panSpeed_ * dt;
            target_ = target_ - (right * input.mouseDelta.x * panFactor);
            target_ = target_ + (up * input.mouseDelta.y * panFactor);
        }

        const math::Vec3 desiredPos = target_ - (forward * distance_);
        position_ = position_ + (desiredPos - position_) * blend;
    }

    math::Mat4 SceneCamera::GetView() const
    {
        return math::LookAtLH(position_, target_, math::Vec3{0.0f, 1.0f, 0.0f});
    }

    math::Mat4 SceneCamera::GetProjection(float aspect) const
    {
        const float safeAspect = (aspect > 0.0f) ? aspect : 1.0f;
        return math::PerspectiveLH(fovRadians_, safeAspect, nearPlane_, farPlane_);
    }

    Ray SceneCamera::BuildPickRay(float ndcX, float ndcY, float aspect) const
    {
        // Ray generation in camera space:
        // dir = normalize(forward + right*x*tan(fov/2)*aspect + up*y*tan(fov/2)).
        math::Vec3 forward{};
        math::Vec3 right{};
        math::Vec3 up{};
        UpdateBasis(forward, right, up);

        const float tanHalfFov = std::tan(fovRadians_ * 0.5f);
        const math::Vec3 dir = math::Normalize(
            forward +
            (right * (ndcX * tanHalfFov * aspect)) +
            (up * (ndcY * tanHalfFov)));

        return Ray{position_, dir};
    }

    void SceneCamera::UpdateBasis(math::Vec3& outForward, math::Vec3& outRight, math::Vec3& outUp) const
    {
        const float cp = std::cos(pitch_);
        outForward = math::Normalize(math::Vec3{
            std::sin(yaw_) * cp,
            std::sin(pitch_),
            std::cos(yaw_) * cp,
        });
        outRight = math::Normalize(math::Cross(math::Vec3{0.0f, 1.0f, 0.0f}, outForward));
        outUp = math::Normalize(math::Cross(outForward, outRight));
    }
}

