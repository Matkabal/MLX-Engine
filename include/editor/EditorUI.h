#pragma once

#include <cstdint>

#include "assets/AssetManager.h"
#include "core/Window.h"
#include "ecs/Entity.h"
#include "editor/GizmoSystem.h"
#include "editor/HierarchyPanel.h"
#include "editor/InspectorPanel.h"
#include "editor/SceneCamera.h"
#include "editor/SceneView.h"
#include "renderer/Dx11Context.h"
#include "scene/Scene.h"

namespace editor
{
    class EditorUI
    {
    public:
        bool Initialize(core::Window& window, renderer::Dx11Context& context);
        void Shutdown();

        void BeginFrame();
        void Update(scene::Scene& scene, assets::AssetManager& assetManager, float dtSeconds);
        void EndFrame();

        const SceneCamera& Camera() const { return camera_; }
        ecs::Entity SelectedEntity() const { return selectedEntity_; }

    private:
        core::Window* window_ = nullptr;
        renderer::Dx11Context* context_ = nullptr;
        SceneCamera camera_{};
        SceneView sceneView_{};
        HierarchyPanel hierarchy_{};
        InspectorPanel inspector_{};
        GizmoSystem gizmo_{};
        ecs::Entity selectedEntity_ = ecs::kInvalidEntity;
    };
}

