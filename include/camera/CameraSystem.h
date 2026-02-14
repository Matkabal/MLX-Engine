#pragma once

#include "camera/CameraComponent.h"
#include "editor/CameraGizmo.h"
#include "input/InputSystem.h"

namespace camera
{
    struct CameraInputState
    {
        float zoomAxis = 0.0f; // +1 near, -1 far
        float panAxis = 0.0f;  // -1 left, +1 right
        float scroll = 0.0f;
        float yawDelta = 0.0f;
        float pitchDelta = 0.0f;
    };

    void UpdateCameraInput(const input::InputFrame& frame, const editor::CameraGizmoDelta& gizmo, CameraInputState& outInput);

    class CameraSystem
    {
    public:
        void Update(CameraComponent& camera, const CameraInputState& input, float aspect, float dtSeconds) const;
    };
}

