#pragma once

#include "math/mat4.h"

namespace math
{
    enum class Handedness
    {
        LeftHanded,
        RightHanded,
    };

    class Camera
    {
    public:
        Camera() = default;

        void SetPosition(const Vec3& p) { position_ = p; }
        void SetTarget(const Vec3& t) { target_ = t; }
        void SetUp(const Vec3& u) { up_ = u; }
        void SetFovRadians(float fov) { fovRadians_ = fov; }
        void SetAspect(float aspect) { aspect_ = (aspect > 0.0f) ? aspect : 1.0f; }
        void SetNearFar(float nearPlane, float farPlane)
        {
            nearPlane_ = (nearPlane > 0.0f) ? nearPlane : 0.1f;
            farPlane_ = (farPlane > nearPlane_) ? farPlane : (nearPlane_ + 100.0f);
        }
        void SetHandedness(Handedness h) { handedness_ = h; }

        const Vec3& GetPosition() const { return position_; }
        const Vec3& GetTarget() const { return target_; }
        float GetAspect() const { return aspect_; }
        Handedness GetHandedness() const { return handedness_; }

        Mat4 GetView() const
        {
            if (handedness_ == Handedness::LeftHanded)
            {
                return LookAtLH(position_, target_, up_);
            }
            return LookAtRH(position_, target_, up_);
        }

        Mat4 GetProjection() const
        {
            if (handedness_ == Handedness::LeftHanded)
            {
                return PerspectiveLH(fovRadians_, aspect_, nearPlane_, farPlane_);
            }
            return PerspectiveRH(fovRadians_, aspect_, nearPlane_, farPlane_);
        }

    private:
        Vec3 position_{0.0f, 0.0f, -3.0f};
        Vec3 target_{0.0f, 0.0f, 0.0f};
        Vec3 up_{0.0f, 1.0f, 0.0f};
        float fovRadians_ = 1.04719755f; // 60 deg
        float aspect_ = 16.0f / 9.0f;
        float nearPlane_ = 0.1f;
        float farPlane_ = 100.0f;
        Handedness handedness_ = Handedness::LeftHanded;
    };
}
