#include "scene/Scene.h"

#include "core/Logger.h"

namespace scene
{
    Scene::Scene()
        : components_(entities_)
    {
        LOG_METHOD("Scene", "Scene");
    }

    ecs::Entity Scene::CreateEntity()
    {
        LOG_METHOD("Scene", "CreateEntity");
        return entities_.Create();
    }

    bool Scene::DestroyEntity(ecs::Entity entity)
    {
        LOG_METHOD("Scene", "DestroyEntity");
        if (!entities_.Destroy(entity))
        {
            return false;
        }
        components_.OnEntityDestroyed(entity);
        return true;
    }

    void Scene::Clear()
    {
        LOG_METHOD("Scene", "Clear");
        components_.Clear();
        entities_.Clear();
    }

    void Scene::Update()
    {
        LOG_METHOD("Scene", "Update");
        transformSystem_.Update(components_);
    }

    std::vector<RenderEntity> Scene::BuildRenderList() const
    {
        std::vector<RenderEntity> out;
        const auto* meshStorage = components_.TryGetStorage<MeshRendererComponent>();
        if (!meshStorage)
        {
            return out;
        }

        const auto& entities = meshStorage->Entities();
        const auto& meshes = meshStorage->Components();
        out.reserve(meshes.size());

        for (size_t i = 0; i < meshes.size(); ++i)
        {
            const auto& mesh = meshes[i];
            if (!mesh.visible || mesh.assetPath.empty())
            {
                continue;
            }

            RenderEntity item{};
            item.entity = entities[i];
            item.assetPath = mesh.assetPath;
            item.meshIndex = mesh.meshIndex;
            item.primitiveIndex = mesh.primitiveIndex;

            const auto* transform = components_.Get<TransformComponent>(item.entity);
            if (transform)
            {
                item.world = transform->world;
            }
            out.push_back(std::move(item));
        }

        return out;
    }
}
