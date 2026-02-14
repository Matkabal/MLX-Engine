#include "editor/SceneView.h"

#if DX11_WITH_IMGUI
#include "imgui.h"
#endif

namespace editor
{
    SceneViewFrameData SceneView::Draw()
    {
        SceneViewFrameData out{};
#if DX11_WITH_IMGUI
        ImGui::Begin("Scene");
        const ImVec2 size = ImGui::GetContentRegionAvail();
        out.viewportWidth = (size.x > 1.0f) ? size.x : 1.0f;
        out.viewportHeight = (size.y > 1.0f) ? size.y : 1.0f;

        ImGui::TextUnformatted("Viewport (DX11 backbuffer)");
        ImGui::InvisibleButton("scene_viewport", size);
        out.hovered = ImGui::IsItemHovered();

        if (out.hovered)
        {
            const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
            out.cameraInput.altDown = ImGui::GetIO().KeyAlt;
            out.cameraInput.leftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            out.cameraInput.rightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
            out.cameraInput.middleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
            out.cameraInput.mouseDelta = math::Vec2{mouseDelta.x, -mouseDelta.y};
            out.cameraInput.wheelDelta = ImGui::GetIO().MouseWheel;

            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();
            const ImVec2 mp = ImGui::GetIO().MousePos;
            const float width = (max.x - min.x > 1.0f) ? (max.x - min.x) : 1.0f;
            const float height = (max.y - min.y > 1.0f) ? (max.y - min.y) : 1.0f;
            out.ndcX = ((mp.x - min.x) / width) * 2.0f - 1.0f;
            out.ndcY = 1.0f - ((mp.y - min.y) / height) * 2.0f;
        }
        ImGui::End();
#endif
        return out;
    }
}

