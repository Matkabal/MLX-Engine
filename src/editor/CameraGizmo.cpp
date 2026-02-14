#include "editor/CameraGizmo.h"

#include <cmath>

#include "editor/ImGuiSupport.h"

#if DX11_WITH_IMGUI
#include "imgui.h"
#endif

namespace editor
{
    CameraGizmoDelta CameraGizmo::DrawCameraGizmo(const camera::CameraComponent& camera) const
    {
        (void)camera;
        CameraGizmoDelta out{};
#if DX11_WITH_IMGUI
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const float size = 92.0f;
        const ImVec2 pos = ImVec2(vp->WorkPos.x + vp->WorkSize.x - size - 16.0f, vp->WorkPos.y + 16.0f);

        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(size, size), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
        ImGui::Begin("CameraGizmo", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings);

        const ImVec2 p0 = ImGui::GetCursorScreenPos();
        const ImVec2 p1 = ImVec2(p0.x + size - 12.0f, p0.y + size - 12.0f);
        const ImVec2 center = ImVec2((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
        const float radius = (size - 20.0f) * 0.5f;

        ImGui::InvisibleButton("camera_gizmo_btn", ImVec2(size - 12.0f, size - 12.0f));
        const bool active = ImGui::IsItemActive();
        if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            const ImVec2 md = ImGui::GetIO().MouseDelta;
            out.yawDelta = md.x * 0.015f;
            out.pitchDelta = -md.y * 0.015f;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddCircle(center, radius, IM_COL32(190, 190, 190, 255), 40, 2.0f);

        auto axisEnd = [&](float x, float y, float z) {
            // Minimal 3D->2D for gizmo icon, driven by current yaw/pitch.
            const float cy = std::cos(camera.yaw);
            const float sy = std::sin(camera.yaw);
            const float cp = std::cos(camera.pitch);
            const float sp = std::sin(camera.pitch);
            const float vx = x * cy + z * sy;
            const float vy = x * sy * sp + y * cp - z * cy * sp;
            return ImVec2(center.x + vx * radius * 0.75f, center.y - vy * radius * 0.75f);
        };

        const ImVec2 ex = axisEnd(1.0f, 0.0f, 0.0f);
        const ImVec2 ey = axisEnd(0.0f, 1.0f, 0.0f);
        const ImVec2 ez = axisEnd(0.0f, 0.0f, 1.0f);
        dl->AddLine(center, ex, IM_COL32(220, 70, 70, 255), 2.0f);
        dl->AddLine(center, ey, IM_COL32(70, 220, 70, 255), 2.0f);
        dl->AddLine(center, ez, IM_COL32(70, 130, 230, 255), 2.0f);
        dl->AddText(ImVec2(ex.x + 2.0f, ex.y - 8.0f), IM_COL32(220, 70, 70, 255), "X");
        dl->AddText(ImVec2(ey.x + 2.0f, ey.y - 8.0f), IM_COL32(70, 220, 70, 255), "Y");
        dl->AddText(ImVec2(ez.x + 2.0f, ez.y - 8.0f), IM_COL32(70, 130, 230, 255), "Z");

        ImGui::End();
        ImGui::PopStyleVar(2);
#endif
        return out;
    }
}

