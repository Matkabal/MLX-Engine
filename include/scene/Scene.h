#pragma once

#include <string>
#include <vector>

#include "ecs/ComponentSystem.h"
#include "ecs/EntityManager.h"
#include "math/mat4.h"
#include "scene/components/MeshRendererComponent.h"
#include "scene/components/TransformComponent.h"
#include "scene/systems/TransformSystem.h"

namespace scene
{
    struct RenderEntity
    {
        ecs::Entity entity = ecs::kInvalidEntity;
        std::string assetPath;
        int meshIndex = -1;
        int primitiveIndex = -1;
        math::Mat4 world = math::Identity();
    };

    class Scene
    {
    public:
        Scene();

        ecs::Entity CreateEntity();
        bool DestroyEntity(ecs::Entity entity);
        void Clear();

        void Update();
        std::vector<RenderEntity> BuildRenderList() const;

        ecs::EntityManager& Entities() { return entities_; }
        const ecs::EntityManager& Entities() const { return entities_; }
        ecs::ComponentSystem& Components() { return components_; }
        const ecs::ComponentSystem& Components() const { return components_; }

    private:
        ecs::EntityManager entities_{};
        ecs::ComponentSystem components_;
        TransformSystem transformSystem_{};
    };
}
