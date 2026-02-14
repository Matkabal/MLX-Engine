#include "editor/EditorUI.h"

#include <windows.h>

#include "core/Logger.h"
#include "editor/ImGuiSupport.h"

#if DX11_WITH_IMGUI
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#endif

namespace editor
{
    bool EditorUI::Initialize(core::Window& window, renderer::Dx11Context& context)
    {
        window_ = &window;
        context_ = &context;
#if DX11_WITH_IMGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(window.GetHandle());
        ImGui_ImplDX11_Init(context.GetDevice(), context.GetDeviceContext());
#else
        LOG_WARN("EditorUI", "Initialize", "Dear ImGui not found in include path. Editor panels disabled.");
#endif
        return true;
    }

    void EditorUI::Shutdown()
    {
#if DX11_WITH_IMGUI
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
#endif
    }

    void EditorUI::BeginFrame()
    {
#if DX11_WITH_IMGUI
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
#endif
    }

    void EditorUI::Update(scene::Scene& scene, assets::AssetManager& assetManager, float dtSeconds)
    {
        (void)assetManager;
        const SceneViewFrameData viewData = sceneView_.Draw();
        if (viewData.hovered)
        {
            camera_.Update(viewData.cameraInput, dtSeconds);
        }

        hierarchy_.Draw(scene, selectedEntity_);
        inspector_.Draw(scene, selectedEntity_);

        GizmoInput gizmoInput{};
#if DX11_WITH_IMGUI
        gizmoInput.leftPressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        gizmoInput.leftHeld = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        gizmoInput.leftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
#endif
        gizmoInput.ndcX = viewData.ndcX;
        gizmoInput.ndcY = viewData.ndcY;
        gizmoInput.aspect = (viewData.viewportHeight > 0.0f) ? (viewData.viewportWidth / viewData.viewportHeight) : 1.0f;
        gizmo_.Update(scene, selectedEntity_, camera_, gizmoInput);
    }

    void EditorUI::EndFrame()
    {
#if DX11_WITH_IMGUI
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
    }
}

