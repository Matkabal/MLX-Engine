#include "editor/InspectorPanel.h"

#include "scene/components/MaterialComponent.h"
#include "scene/components/MeshRendererComponent.h"
#include "scene/components/NameComponent.h"
#include "scene/components/TransformComponent.h"

#if DX11_WITH_IMGUI
#include "imgui.h"
#endif

namespace editor
{
    void InspectorPanel::Draw(scene::Scene& scene, ecs::Entity selectedEntity)
    {
#if DX11_WITH_IMGUI
        if (!ImGui::Begin("Inspector"))
        {
            ImGui::End();
            return;
        }

        if (selectedEntity == ecs::kInvalidEntity || !scene.Entities().IsAlive(selectedEntity))
        {
            ImGui::TextUnformatted("Select an entity.");
            ImGui::End();
            return;
        }

        ImGui::Text("Entity: %u", static_cast<unsigned int>(selectedEntity));

        if (auto* name = scene.Components().Get<scene::NameComponent>(selectedEntity))
        {
            char nameBuffer[128]{};
            const size_t count = (name->value.size() < sizeof(nameBuffer) - 1u) ? name->value.size() : (sizeof(nameBuffer) - 1u);
            for (size_t i = 0; i < count; ++i)
            {
                nameBuffer[i] = name->value[i];
            }
            if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
            {
                name->value = nameBuffer;
            }
        }
        else if (ImGui::Button("Add NameComponent"))
        {
            scene.Components().Add<scene::NameComponent>(selectedEntity, scene::NameComponent{"Entity"});
        }

        if (auto* transform = scene.Components().Get<scene::TransformComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Position", &transform->local.position.x, 0.05f);
                ImGui::DragFloat3("Rotation", &transform->local.rotationRadians.x, 0.02f);
                ImGui::DragFloat3("Scale", &transform->local.scale.x, 0.02f);
            }
        }

        if (auto* mesh = scene.Components().Get<scene::MeshRendererComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("MeshComponent", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Asset: %s", mesh->assetPath.c_str());
                ImGui::DragInt("Mesh Index", &mesh->meshIndex, 1.0f, -1, 1024);
                ImGui::DragInt("Primitive Index", &mesh->primitiveIndex, 1.0f, -1, 1024);
                ImGui::Checkbox("Visible", &mesh->visible);
                if (ImGui::Button("Remove MeshComponent"))
                {
                    scene.Components().Remove<scene::MeshRendererComponent>(selectedEntity);
                }
            }
        }
        else if (ImGui::Button("Add MeshComponent"))
        {
            scene.Components().Add<scene::MeshRendererComponent>(selectedEntity, scene::MeshRendererComponent{});
        }

        if (auto* material = scene.Components().Get<scene::MaterialComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("MaterialComponent", ImGuiTreeNodeFlags_DefaultOpen))
            {
                char materialId[128]{};
                const std::string& src = material->materialId;
                const size_t count = (src.size() < sizeof(materialId) - 1u) ? src.size() : (sizeof(materialId) - 1u);
                for (size_t i = 0; i < count; ++i)
                {
                    materialId[i] = src[i];
                }
                if (ImGui::InputText("Material", materialId, sizeof(materialId)))
                {
                    material->materialId = materialId;
                }
                if (ImGui::Button("Remove MaterialComponent"))
                {
                    scene.Components().Remove<scene::MaterialComponent>(selectedEntity);
                }
            }
        }
        else if (ImGui::Button("Add MaterialComponent"))
        {
            scene.Components().Add<scene::MaterialComponent>(selectedEntity, scene::MaterialComponent{});
        }

        ImGui::End();
#else
        (void)scene;
        (void)selectedEntity;
#endif
    }
}
