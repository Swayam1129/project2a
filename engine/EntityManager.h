#pragma once

#include <cassert>
#include <iostream>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "Component.h"
#include "Entity.h"

namespace ECSEngine {

template <typename... Components>
class EntityManager {
public:
    static constexpr size_t kNullComponent = std::numeric_limits<size_t>::max();

    // Creates a new entity and returns its id. Ids of removed entities are reused.
    // The id stays valid until RemoveEntity is called on it.
    [[nodiscard]] EntityID CreateEntity(const std::string& name) {
        EntityID id;
        if (!mFreeEntities.empty()) {
            id = mFreeEntities.back();
            mFreeEntities.pop_back();
            mEntities[id] = Entity(id, name);
            mEntityValid[id] = true;
        } else {
            id = mEntities.size();
            mEntities.push_back(Entity(id, name));
            mEntityValid.push_back(true);
        }
        return id;
    }

    // Removes the entity and all of its components. Its id becomes invalid and may be reused.
    void RemoveEntity(EntityID entity) {
        if (!IsValid(entity)) return;
        (RemoveComponent<Components>(entity), ...);
        mEntityValid[entity] = false;
        mFreeEntities.push_back(entity);
    }

    // true if this id refers to a live entity (use this to skip holes while iterating)
    // Returns true if the id refers to a live entity. Use this to skip removed slots while iterating.
    bool IsValid(EntityID entity) const {
        return entity < mEntities.size() && mEntityValid[entity];
    }

    // Adds (or overwrites) a component of type T on a live entity. The component is copied in.
    template <typename T>
    void AddComponent(EntityID entity, T component) {
        assert(IsValid(entity));
        auto& s = GetComponentStorage<T>();
        if (HasComponent<T>(entity)) {
            s.mStorage[s.mEntityToIndex[entity]] = component;
            return;
        }
        size_t index;
        if (!s.mFreeList.empty()) {
            index = s.mFreeList.back();
            s.mFreeList.pop_back();
            s.mStorage[index] = component;
            s.mValid[index] = true;
        } else {
            index = s.mStorage.size();
            s.mStorage.push_back(component);
            s.mValid.push_back(true);
        }
        if (s.mEntityToIndex.size() <= entity) s.mEntityToIndex.resize(entity + 1, kNullComponent);
        s.mEntityToIndex[entity] = index;
    }

    // Removes the T component from the entity, if it has one.
    template <typename T>
    void RemoveComponent(EntityID entity) {
        if (!HasComponent<T>(entity)) return;
        auto& s = GetComponentStorage<T>();
        size_t index = s.mEntityToIndex[entity];
        s.mValid[index] = false;
        s.mFreeList.push_back(index);
        s.mEntityToIndex[entity] = kNullComponent;
    }

    // Returns true if the entity currently has a T component.
    template <typename T>
    bool HasComponent(EntityID entity) const {
        const auto& s = std::get<ComponentStorage<T>>(mStorages);
        return entity < s.mEntityToIndex.size() && s.mEntityToIndex[entity] != kNullComponent;
    }

    // Returns a reference to the entity's T component. The entity must have one (asserted).
    // The reference is only valid until the next AddComponent<T> call, which may resize the storage.
    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(HasComponent<T>(entity));
        auto& s = GetComponentStorage<T>();
        return s.mStorage[s.mEntityToIndex[entity]];
    }

    // Direct access to all T components (for systems that loop over one component type).
    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        return std::get<ComponentStorage<T>>(mStorages);
    }

    using tEntity = std::vector<Entity>;
    using const_iterator = tEntity::const_iterator;
    const_iterator cbegin() const { return mEntities.cbegin(); }
    const_iterator cend() const { return mEntities.cend(); }
    const_iterator begin() const { return mEntities.cbegin(); }
    const_iterator end() const { return mEntities.cend(); }

private:
    std::vector<Entity> mEntities;       // id == index; removed entities leave holes
    std::vector<bool> mEntityValid;      // mEntityValid[id] == false means hole
    std::vector<EntityID> mFreeEntities; // ids of holes, reused by CreateEntity
    std::tuple<ComponentStorage<Components>...> mStorages;  // one storage per component type
};

}  // namespace ECSEngine
