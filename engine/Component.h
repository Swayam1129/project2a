#pragma once

#include <vector>

namespace ECSEngine {

template <typename T>
class ComponentStorage {
public:
    std::vector<T> mStorage;
    std::vector<bool> mValid;       // Quickly track if a component is valid
    std::vector<size_t> mFreeList;  // Avoid scanning for free slots
    std::vector<size_t> mEntityToIndex;  // entity id -> index in mStorage (or kNullComponent)
};

}  // namespace ECSEngine
