#pragma once

#include <string>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

// Shows the score: sets each digit entity's sprite to the matching digit (000 to 999).
// Loops over the ScoreComponent storage only.
template <typename... Components>
class ScoreSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("ScoreSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& scores = em.template GetComponentStorage<ScoreComponent>();
        for (size_t s = 0; s < scores.mStorage.size(); ++s) {
            if (!scores.mValid[s]) continue;
            const ScoreComponent& score = scores.mStorage[s];
            int value = score.score % 1000;
            int digits[3] = {value / 100, (value / 10) % 10, value % 10};
            for (int i = 0; i < 3; ++i) {
                EntityID d = score.digitID[i];
                if (em.IsValid(d) && em.template HasComponent<SpriteComponent>(d))
                    em.template GetComponent<SpriteComponent>(d).textureID =
                        score.digitTextureID[digits[i]];
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
