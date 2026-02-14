#include "camera/CameraSystem.h"

#include <algorithm>
#include <cmath>

namespace camera
{
    void UpdateCameraInput(const input::InputFrame& frame, const editor::CameraGizmoDelta& gizmo, CameraInputState& outInput)
    {
        outInput = CameraInputState{};
        if (frame.up)
        {
            outInput.zoomAxis += 1.0f;
        }
        if (frame.down)
        {
            outInput.zoomAxis -= 1.0f;
        }
        if (frame.left)
        {
            outInput.panAxis -= 1.0f;
        }
        if (frame.right)
        {
            outInput.panAxis += 1.0f;
        }
        outInput.scroll = frame.wheelDelta;
        outInput.yawDelta = gizmo.yawDelta;
        outInput.pitchDelta = gizmo.pitchDelta;
    }

    void CameraSystem::Update(CameraComponent& camera, const CameraInputState& input, float aspect, float dtSeconds) const
    {
        const float dt = (dtSeconds > 0.0f) ? dtSeconds : (1.0f / 60.0f);
        const float safeAspect = (aspect > 0.0f) ? aspect : 1.0f;

        // Zoom adjusts orbital radius to target.
        const float zoomDelta = (input.zoomAxis * camera.zoomSpeed + input.scroll * camera.zoomSpeed * 2.0f) * dt;
        camera.distance -= zoomDelta;
        camera.distance = std::clamp(camera.distance, camera.minDistance, camera.maxDistance);

        // Pan on camera right vector, moving both camera target and orbit center.
        const float cp = std::cos(camera.pitch);
        const math::Vec3 forward = math::Normalize(math::Vec3{
            std::sin(camera.yaw) * cp,
            std::sin(camera.pitch),
            std::cos(camera.yaw) * cp
        });
        math::Vec3 right = math::Normalize(math::Cross(math::Vec3{0.0f, 1.0f, 0.0f}, forward));
        if (math::Length(right) <= 1e-5f)
        {
            right = math::Vec3{1.0f, 0.0f, 0.0f};
        }
        camera.target = camera.target + right * (input.panAxis * camera.panSpeed * dt);

        // Orbital rotation from gizmo drag.
        camera.yaw += input.yawDelta * camera.orbitSpeed;
        camera.pitch += input.pitchDelta * camera.orbitSpeed;
        camera.pitch = std::clamp(camera.pitch, -1.48352986f, 1.48352986f);

        // Orbital camera position (spec formula).
        camera.position.x = camera.target.x + camera.distance * std::cos(camera.pitch) * std::sin(camera.yaw);
        camera.position.y = camera.target.y + camera.distance * std::sin(camera.pitch);
        camera.position.z = camera.target.z + camera.distance * std::cos(camera.pitch) * std::cos(camera.yaw);

        camera.viewMatrix = math::LookAtLH(camera.position, camera.target, math::Vec3{0.0f, 1.0f, 0.0f});
        camera.projectionMatrix = math::PerspectiveLH(
            camera.fovRadians,
            safeAspect,
            camera.nearPlane,
            camera.farPlane);
    }
}

