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

// Moves every entity by its velocity (pixels per frame).
template <typename... Components>
class MovementSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("MovementSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        for (const Entity& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id) || !em.template HasComponent<MovementComponent>(id) ||
                !em.template HasComponent<LocationComponent>(id))
                continue;
            const Point2D& v = em.template GetComponent<MovementComponent>(id).velocity;
            Point2D& loc = em.template GetComponent<LocationComponent>(id).loc;
            loc.x += v.x;
            loc.y += v.y;
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
