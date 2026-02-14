#pragma once

#include <cstdint>

namespace ecs
{
    using Entity = uint32_t;

    constexpr uint32_t kEntityIndexBits = 20;
    constexpr uint32_t kEntityGenerationBits = 12;
    constexpr uint32_t kEntityIndexMask = (1u << kEntityIndexBits) - 1u;
    constexpr uint32_t kEntityGenerationMask = (1u << kEntityGenerationBits) - 1u;
    constexpr Entity kInvalidEntity = 0;

    inline Entity MakeEntity(uint32_t index, uint32_t generation)
    {
        return (index & kEntityIndexMask) | ((generation & kEntityGenerationMask) << kEntityIndexBits);
    }

    inline uint32_t GetEntityIndex(Entity entity)
    {
        return entity & kEntityIndexMask;
    }

    inline uint32_t GetEntityGeneration(Entity entity)
    {
        return (entity >> kEntityIndexBits) & kEntityGenerationMask;
    }
}
