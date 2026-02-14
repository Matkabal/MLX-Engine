#pragma once

#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ecs/Entity.h"
#include "ecs/EntityManager.h"

namespace ecs
{
    class IComponentStorage
    {
    public:
        virtual ~IComponentStorage() = default;
        virtual void Remove(Entity entity) = 0;
        virtual void Clear() = 0;
    };

    template <typename T>
    class ComponentStorage final : public IComponentStorage
    {
    public:
        T* Add(Entity entity, T component)
        {
            const uint32_t index = GetEntityIndex(entity);
            EnsureSparseSize(index + 1);

            uint32_t denseIndex = sparse_[index];
            if (denseIndex != 0u)
            {
                components_[denseIndex - 1u] = std::move(component);
                return &components_[denseIndex - 1u];
            }

            entities_.push_back(entity);
            components_.push_back(std::move(component));
            sparse_[index] = static_cast<uint32_t>(components_.size());
            return &components_.back();
        }

        bool Has(Entity entity) const
        {
            const uint32_t index = GetEntityIndex(entity);
            if (index >= sparse_.size())
            {
                return false;
            }
            return sparse_[index] != 0u;
        }

        T* Get(Entity entity)
        {
            const uint32_t index = GetEntityIndex(entity);
            if (index >= sparse_.size())
            {
                return nullptr;
            }
            const uint32_t denseIndex = sparse_[index];
            if (denseIndex == 0u)
            {
                return nullptr;
            }
            return &components_[denseIndex - 1u];
        }

        const T* Get(Entity entity) const
        {
            const uint32_t index = GetEntityIndex(entity);
            if (index >= sparse_.size())
            {
                return nullptr;
            }
            const uint32_t denseIndex = sparse_[index];
            if (denseIndex == 0u)
            {
                return nullptr;
            }
            return &components_[denseIndex - 1u];
        }

        void Remove(Entity entity) override
        {
            const uint32_t index = GetEntityIndex(entity);
            if (index >= sparse_.size())
            {
                return;
            }
            const uint32_t denseIndex = sparse_[index];
            if (denseIndex == 0u)
            {
                return;
            }

            const uint32_t removeAt = denseIndex - 1u;
            const uint32_t lastIndex = static_cast<uint32_t>(components_.size() - 1u);
            if (removeAt != lastIndex)
            {
                components_[removeAt] = std::move(components_[lastIndex]);
                entities_[removeAt] = entities_[lastIndex];
                sparse_[GetEntityIndex(entities_[removeAt])] = removeAt + 1u;
            }

            components_.pop_back();
            entities_.pop_back();
            sparse_[index] = 0u;
        }

        void Clear() override
        {
            components_.clear();
            entities_.clear();
            sparse_.clear();
        }

        const std::vector<Entity>& Entities() const { return entities_; }
        const std::vector<T>& Components() const { return components_; }
        std::vector<T>& Components() { return components_; }

    private:
        void EnsureSparseSize(size_t size)
        {
            if (sparse_.size() < size)
            {
                sparse_.resize(size, 0u);
            }
        }

    private:
        std::vector<Entity> entities_{};
        std::vector<T> components_{};
        std::vector<uint32_t> sparse_{};
    };

    class ComponentSystem
    {
    public:
        explicit ComponentSystem(EntityManager& entityManager)
            : entityManager_(entityManager) {}

        template <typename T>
        T* Add(Entity entity, T component)
        {
            if (!entityManager_.IsAlive(entity))
            {
                return nullptr;
            }
            return GetOrCreateStorage<T>().Add(entity, std::move(component));
        }

        template <typename T>
        bool Has(Entity entity) const
        {
            const auto* storage = FindStorage<T>();
            return storage ? storage->Has(entity) : false;
        }

        template <typename T>
        T* Get(Entity entity)
        {
            auto* storage = FindStorage<T>();
            return storage ? storage->Get(entity) : nullptr;
        }

        template <typename T>
        const T* Get(Entity entity) const
        {
            const auto* storage = FindStorage<T>();
            return storage ? storage->Get(entity) : nullptr;
        }

        template <typename T>
        void Remove(Entity entity)
        {
            auto* storage = FindStorage<T>();
            if (storage)
            {
                storage->Remove(entity);
            }
        }

        void OnEntityDestroyed(Entity entity)
        {
            for (auto& [_, storage] : storages_)
            {
                storage->Remove(entity);
            }
        }

        void Clear()
        {
            for (auto& [_, storage] : storages_)
            {
                storage->Clear();
            }
        }

        template <typename T>
        ComponentStorage<T>* TryGetStorage()
        {
            return FindStorage<T>();
        }

        template <typename T>
        const ComponentStorage<T>* TryGetStorage() const
        {
            return FindStorage<T>();
        }

    private:
        template <typename T>
        ComponentStorage<T>* FindStorage()
        {
            auto it = storages_.find(std::type_index(typeid(T)));
            if (it == storages_.end())
            {
                return nullptr;
            }
            return static_cast<ComponentStorage<T>*>(it->second.get());
        }

        template <typename T>
        const ComponentStorage<T>* FindStorage() const
        {
            auto it = storages_.find(std::type_index(typeid(T)));
            if (it == storages_.end())
            {
                return nullptr;
            }
            return static_cast<const ComponentStorage<T>*>(it->second.get());
        }

        template <typename T>
        ComponentStorage<T>& GetOrCreateStorage()
        {
            auto* found = FindStorage<T>();
            if (found)
            {
                return *found;
            }

            const auto key = std::type_index(typeid(T));
            auto storage = std::make_unique<ComponentStorage<T>>();
            auto* ptr = storage.get();
            storages_[key] = std::move(storage);
            return *ptr;
        }

    private:
        EntityManager& entityManager_;
        std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> storages_{};
    };
}
