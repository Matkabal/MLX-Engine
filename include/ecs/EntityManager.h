#pragma once

#include <cstdint>
#include <vector>

#include "ecs/Entity.h"

namespace ecs
{
    class EntityManager
    {
    public:
        Entity Create();
        bool Destroy(Entity entity);
        bool IsAlive(Entity entity) const;
        void Clear();

    private:
        std::vector<uint32_t> generations_{};
        std::vector<uint8_t> aliveFlags_{};
        std::vector<uint32_t> freeIndices_{};
    };
}
