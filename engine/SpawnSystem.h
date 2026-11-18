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
#include <cmath>
#include <random>
#include <vector>

namespace ECSEngine {

const float kStarSize = 40.0f;          // drawn size of a star (pixels)
const float kStarSpeed = 7.0f;          // launch speed of a spawned star
const float kStarGravity = 0.35f;       // stars fall more gently than the player
const float kSpawnDistance = 52.0f;     // spawn this far from the spawner center (outside it)

// Counts down every SpawnComponent and, when its timer runs out, creates a star next to the
// spawner moving in a random direction. New entities are created after the loop over
// entities, because creating entities can move the entity list in memory.
template <typename... Components>
class SpawnSystem : public System<Components...> {
public:
    SpawnSystem() : mRandom(std::random_device{}()) {}

    const std::string& GetName() const override {
        static std::string name("SpawnSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        struct Pending {
            Point2D center;
            SpriteID sprite;
        };
        std::vector<Pending> pending;

        for (const Entity& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id) || !em.template HasComponent<SpawnComponent>(id) ||
                !em.template HasComponent<LocationComponent>(id))
                continue;
            SpawnComponent& spawn = em.template GetComponent<SpawnComponent>(id);
            if (spawn.totalSpawns <= 0 || --spawn.timeToNextSpawn > 0) continue;
            spawn.timeToNextSpawn = spawn.timeBetweenSpawns;
            --spawn.totalSpawns;

            Point2D center = em.template GetComponent<LocationComponent>(id).loc;
            if (em.template HasComponent<SpriteComponent>(id)) {
                const Rect& b = em.template GetComponent<SpriteComponent>(id).bounds;
                center.x += b.topLeft.x + b.width / 2.0f;
                center.y += b.topLeft.y + b.height / 2.0f;
            }
            if (spawn.spawnType == "star") pending.push_back({center, spawn.spawnTextureID});
        }

        for (const Pending& p : pending) SpawnStar(em, p.center, p.sprite);
        return kContinue;
    }

private:
    template <typename EM>
    void SpawnStar(EM& em, const Point2D& spawnerCenter, SpriteID sprite) {
        std::uniform_real_distribution<float> angle(0.0f, 6.2831853f);
        float a = angle(mRandom);
        Point2D dir(std::cos(a), std::sin(a));
        Point2D topLeft(spawnerCenter.x + dir.x * kSpawnDistance - kStarSize / 2.0f,
                        spawnerCenter.y + dir.y * kSpawnDistance - kStarSize / 2.0f);

        EntityID star = em.CreateEntity("star");
        em.AddComponent(star, LocationComponent{topLeft});
        em.AddComponent(star, MovementComponent{kStarSpeed * dir});
        em.AddComponent(star, GravityComponent(Point2D(0.0f, kStarGravity)));
        em.AddComponent(star, SpriteComponent{sprite, {{0.0f, 0.0f}, kStarSize, kStarSize}, true});
        CollisionComponent col{};
        col.staticEntity = 0;
        col.collisionBox = {{4.0f, 4.0f}, kStarSize - 8.0f, kStarSize - 8.0f};
        col.previousCollisionBox = Offset(col.collisionBox, topLeft);
        em.AddComponent(star, col);
    }

    std::mt19937 mRandom;
};

}  // namespace ECSEngine
