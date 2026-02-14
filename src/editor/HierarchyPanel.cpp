#include "editor/HierarchyPanel.h"

#include <unordered_map>
#include <vector>

#include "scene/components/TransformComponent.h"
#include "scene/components/NameComponent.h"

#if DX11_WITH_IMGUI
#include "imgui.h"
#endif

namespace editor
{
    namespace
    {
#if DX11_WITH_IMGUI
        void DrawNodeRecursive(
            ecs::Entity entity,
            scene::Scene& sceneRef,
            const std::unordered_map<ecs::Entity, std::vector<ecs::Entity>>& childrenByParent,
            ecs::Entity& selectedEntity)
        {
            const bool selected = (selectedEntity == entity);
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            if (selected)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            const auto it = childrenByParent.find(entity);
            if (it == childrenByParent.end() || it->second.empty())
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            std::string label = "Entity " + std::to_string(entity);
            if (const auto* name = sceneRef.Components().Get<scene::NameComponent>(entity))
            {
                if (!name->value.empty())
                {
                    label = name->value + " (" + std::to_string(entity) + ")";
                }
            }
            const bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)), flags, "%s", label.c_str());
            if (ImGui::IsItemClicked())
            {
                selectedEntity = entity;
            }

            if (opened)
            {
                if (it != childrenByParent.end())
                {
                    for (const ecs::Entity child : it->second)
                    {
                        DrawNodeRecursive(child, sceneRef, childrenByParent, selectedEntity);
                    }
                }
                ImGui::TreePop();
            }
        }
#endif
    }

    void HierarchyPanel::Draw(scene::Scene& scene, ecs::Entity& selectedEntity)
    {
#if DX11_WITH_IMGUI
        if (!ImGui::Begin("Hierarchy"))
        {
            ImGui::End();
            return;
        }

        const auto* storage = scene.Components().TryGetStorage<scene::TransformComponent>();
        if (!storage)
        {
            ImGui::TextUnformatted("No entities.");
            ImGui::End();
            return;
        }

        std::unordered_map<ecs::Entity, std::vector<ecs::Entity>> childrenByParent;
        std::vector<ecs::Entity> roots;

        const auto& entities = storage->Entities();
        const auto& transforms = storage->Components();
        for (size_t i = 0; i < entities.size() && i < transforms.size(); ++i)
        {
            const ecs::Entity e = entities[i];
            const ecs::Entity p = transforms[i].parent;
            if (p == ecs::kInvalidEntity || !scene.Entities().IsAlive(p))
            {
                roots.push_back(e);
            }
            else
            {
                childrenByParent[p].push_back(e);
            }
        }

        if (ImGui::TreeNodeEx("SceneRoot", ImGuiTreeNodeFlags_DefaultOpen, "Scene"))
        {
            if (ImGui::TreeNodeEx("AssetsRoot", ImGuiTreeNodeFlags_DefaultOpen, "Assets"))
            {
                for (const ecs::Entity root : roots)
                {
                    DrawNodeRecursive(root, scene, childrenByParent, selectedEntity);
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
        ImGui::End();
#else
        (void)scene;
        (void)selectedEntity;
#endif
    }
}
