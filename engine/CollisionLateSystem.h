#pragma once

#include <string>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SoundNames.h"
#include "SystemManager.h"

namespace ECSEngine {

// Runs first each frame. Saves every moving entity's collision box (in world coordinates)
// into previousCollisionBox, so CollisionSystem knows where the entity was before it moved.
template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        for (const Entity& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id) || !em.template HasComponent<CollisionComponent>(id) ||
                !em.template HasComponent<LocationComponent>(id))
                continue;
            auto& col = em.template GetComponent<CollisionComponent>(id);
            if (col.staticEntity) continue;
            const Point2D& loc = em.template GetComponent<LocationComponent>(id).loc;
            col.previousCollisionBox = Offset(col.collisionBox, loc);
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
