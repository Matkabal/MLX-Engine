#pragma once

#include "math/mat4.h"
#include "math/vec2.h"
#include "math/vec3.h"

namespace editor
{
    struct SceneCameraInput
    {
        bool altDown = false;
        bool leftDown = false;
        bool middleDown = false;
        bool rightDown = false;
        math::Vec2 mouseDelta{};
        float wheelDelta = 0.0f;
    };

    struct Ray
    {
        math::Vec3 origin{};
        math::Vec3 direction{};
    };

    class SceneCamera
    {
    public:
        void Update(const SceneCameraInput& input, float dtSeconds);

        math::Mat4 GetView() const;
        math::Mat4 GetProjection(float aspect) const;
        Ray BuildPickRay(float ndcX, float ndcY, float aspect) const;

        const math::Vec3& GetPosition() const { return position_; }
        const math::Vec3& GetTarget() const { return target_; }

        void SetOrbitSpeed(float value) { orbitSpeed_ = value; }
        void SetZoomSpeed(float value) { zoomSpeed_ = value; }
        void SetPanSpeed(float value) { panSpeed_ = value; }
        void SetSmoothing(float value) { smoothing_ = value; }

    private:
        void UpdateBasis(math::Vec3& outForward, math::Vec3& outRight, math::Vec3& outUp) const;

    private:
        math::Vec3 target_{0.0f, 0.0f, 0.0f};
        math::Vec3 position_{0.0f, 1.5f, -4.0f};
        float yaw_ = 0.0f;
        float pitch_ = 0.35f;
        float distance_ = 4.5f;
        float fovRadians_ = 1.04719755f;
        float nearPlane_ = 0.05f;
        float farPlane_ = 500.0f;

        float orbitSpeed_ = 2.4f;
        float zoomSpeed_ = 10.0f;
        float panSpeed_ = 1.8f;
        float smoothing_ = 16.0f;
    };
}

