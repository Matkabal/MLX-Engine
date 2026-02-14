#include "scene/systems/TransformSystem.h"

#include <functional>
#include <unordered_map>

#include "core/Logger.h"

namespace scene
{
    void TransformSystem::Update(ecs::ComponentSystem& components) const
    {
        LOG_METHOD("TransformSystem", "Update");
        auto* storage = components.TryGetStorage<TransformComponent>();
        if (!storage)
        {
            return;
        }

        std::unordered_map<ecs::Entity, uint8_t> visit{};
        const auto& entities = storage->Entities();
        visit.reserve(entities.size());

        const std::function<void(ecs::Entity)> computeWorld = [&](ecs::Entity e) {
            auto* t = components.Get<TransformComponent>(e);
            if (!t)
            {
                return;
            }
            const auto it = visit.find(e);
            if (it != visit.end())
            {
                if (it->second == 2u)
                {
                    return;
                }
                if (it->second == 1u)
                {
                    t->world = t->local.ToMatrix();
                    visit[e] = 2u;
                    return;
                }
            }

            visit[e] = 1u;
            const math::Mat4 local = t->local.ToMatrix();
            if (t->parent != ecs::kInvalidEntity)
            {
                computeWorld(t->parent);
                if (const auto* parent = components.Get<TransformComponent>(t->parent))
                {
                    t->world = math::Multiply(local, parent->world);
                }
                else
                {
                    t->world = local;
                }
            }
            else
            {
                t->world = local;
            }
            visit[e] = 2u;
        };

        for (ecs::Entity e : entities)
        {
            computeWorld(e);
        }
    }
}
