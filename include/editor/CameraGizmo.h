#pragma once

#include "camera/CameraComponent.h"

namespace editor
{
    struct CameraGizmoDelta
    {
        float yawDelta = 0.0f;
        float pitchDelta = 0.0f;
    };

    class CameraGizmo
    {
    public:
        CameraGizmoDelta DrawCameraGizmo(const camera::CameraComponent& camera) const;
    };
}

