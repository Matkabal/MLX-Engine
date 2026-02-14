#include "ecs/EntityManager.h"

#include "core/Logger.h"

namespace ecs
{
    Entity EntityManager::Create()
    {
        LOG_METHOD("EntityManager", "Create");
        uint32_t index = 0;
        if (!freeIndices_.empty())
        {
            index = freeIndices_.back();
            freeIndices_.pop_back();
            aliveFlags_[index] = 1;
        }
        else
        {
            index = static_cast<uint32_t>(generations_.size());
            generations_.push_back(1u);
            aliveFlags_.push_back(1u);
        }

        return MakeEntity(index, generations_[index]);
    }

    bool EntityManager::Destroy(Entity entity)
    {
        LOG_METHOD("EntityManager", "Destroy");
        const uint32_t index = GetEntityIndex(entity);
        if (index >= generations_.size())
        {
            return false;
        }
        if (!IsAlive(entity))
        {
            return false;
        }

        aliveFlags_[index] = 0u;
        generations_[index] = (generations_[index] + 1u) & kEntityGenerationMask;
        if (generations_[index] == 0u)
        {
            generations_[index] = 1u;
        }
        freeIndices_.push_back(index);
        return true;
    }

    bool EntityManager::IsAlive(Entity entity) const
    {
        const uint32_t index = GetEntityIndex(entity);
        if (index >= generations_.size())
        {
            return false;
        }
        if (aliveFlags_[index] == 0u)
        {
            return false;
        }
        return generations_[index] == GetEntityGeneration(entity);
    }

    void EntityManager::Clear()
    {
        LOG_METHOD("EntityManager", "Clear");
        generations_.clear();
        aliveFlags_.clear();
        freeIndices_.clear();
    }
}
