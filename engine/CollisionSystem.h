#pragma once

#include <string>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundNames.h"
#include "SystemManager.h"
#include <algorithm>
#include <vector>

namespace ECSEngine {

const float kShakeMinSpeed = 6.0f;    // slower impacts do not shake the camera
const float kShakeStrength = 0.6f;    // shake offset (pixels) per unit of impact speed
const int kShakeFrames = 10;
// The map's spike hitbox (y 10..54) is shorter than the drawn spikes (y 0..64), so being
// stopped by a neighbouring tile could leave you visually touching spikes without dying.
// Any entity within this many pixels above/below a damaging box counts as touching it.
const float kSpikeTouchReach = 11.0f;
const float kBounce = 0.7f;         // speed kept by a star after each bounce
const float kStarRestSpeed = 1.5f;  // slower vertical bounces than this just stop on the ground
const float kStarRollFriction = 0.9f;

// Resolves collisions between moving entities and static entities (tiles).
// For each moving entity it replays this frame's movement in two steps, starting from
// previousCollisionBox (saved by CollisionLateSystem):
//   1. move sideways  -> if it hits a tile, stop at the tile edge (onLeftWall / onRightWall)
//   2. move up/down   -> if it hits a tile, stop at the tile edge (onGround / onCeiling)
// Doing the two axes separately stops entities from catching on tile corners.
// Touching a damaging tile (spikes) sends an entity with a RespawnComponent back to its
// respawn point. A landing sound plays on the frame the player lands.
// When the player hits a tile fast enough, the camera shakes in the direction of the hit,
// more strongly the faster the player was moving.
//
// Moving entities without an InputComponent (stars) bounce off tiles instead of stopping,
// losing speed each bounce, and bounce off each other. When the player touches a star the
// star is removed, the score goes up by one, and a sound plays.
template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();

        // Find the camera shake (if any) so hits can trigger it.
        mShake = nullptr;
        for (const Entity& e : em) {
            if (em.IsValid(e.GetID()) && em.template HasComponent<CameraShake>(e.GetID())) {
                mShake = &em.template GetComponent<CameraShake>(e.GetID());
                break;
            }
        }

        // Gather static boxes once per frame.
        mStatic.clear();
        for (const Entity& e : em) {
            EntityID id = e.GetID();
            if (!IsCollider(em, id)) continue;
            const auto& col = em.template GetComponent<CollisionComponent>(id);
            if (!col.staticEntity) continue;
            const Point2D& loc = em.template GetComponent<LocationComponent>(id).loc;
            mStatic.push_back({Offset(col.collisionBox, loc), col.doesDamage != 0});
        }

        for (const Entity& e : em) {
            EntityID id = e.GetID();
            if (!IsCollider(em, id) || !em.template HasComponent<MovementComponent>(id)) continue;
            auto& col = em.template GetComponent<CollisionComponent>(id);
            if (col.staticEntity) continue;
            ResolveStatic(engine, id, col);
        }

        ResolveDynamicPairs(engine);
        return kContinue;
    }

private:
    struct StaticBox {
        Rect box;
        bool damage;
    };

    template <typename EM>
    static bool IsCollider(EM& em, EntityID id) {
        return em.IsValid(id) && em.template HasComponent<CollisionComponent>(id) &&
               em.template HasComponent<LocationComponent>(id);
    }

    void ResolveStatic(EngineView<Components...>* engine, EntityID id, CollisionComponent& col) {
        auto& em = engine->GetEntityManager();
        Point2D& loc = em.template GetComponent<LocationComponent>(id).loc;
        Point2D& v = em.template GetComponent<MovementComponent>(id).velocity;

        bool isPlayer = em.template HasComponent<InputComponent>(id);
        bool bouncy = !isPlayer;
        bool wasOnGround = col.onGround;
        col.onGround = col.onCeiling = col.onLeftWall = col.onRightWall = 0;
        bool hurt = false;
        float hitX = 0, hitY = 0;  // signed speed at impact, for screen shake

        Rect now = Offset(col.collisionBox, loc);
        Rect box = col.previousCollisionBox;
        float dx = now.topLeft.x - box.topLeft.x;
        float dy = now.topLeft.y - box.topLeft.y;

        // 1. sideways
        box.topLeft.x += dx;
        for (const StaticBox& s : mStatic) {
            if (!Overlaps(box, s.box)) continue;
            hurt = hurt || s.damage;
            if (dx > 0) {
                box.topLeft.x = s.box.topLeft.x - box.width;
                col.onRightWall = 1;
            } else if (dx < 0) {
                box.topLeft.x = s.box.topLeft.x + s.box.width;
                col.onLeftWall = 1;
            }
            if (dx != 0) {
                hitX = dx;
                v.x = bouncy ? -v.x * kBounce : 0.0f;
            }
        }

        // 2. up / down
        box.topLeft.y += dy;
        for (const StaticBox& s : mStatic) {
            if (!Overlaps(box, s.box)) continue;
            hurt = hurt || s.damage;
            if (dy > 0) {
                box.topLeft.y = s.box.topLeft.y - box.height;
                col.onGround = 1;
            } else if (dy < 0) {
                box.topLeft.y = s.box.topLeft.y + s.box.height;
                col.onCeiling = 1;
            }
            if (dy != 0) {
                hitY = dy;
                v.y = bouncy ? -v.y * kBounce : 0.0f;
                if (bouncy && dy > 0) {
                    if (v.y > -kStarRestSpeed) v.y = 0;  // too slow to bounce again: rest
                    v.x *= kStarRollFriction;
                }
            }
        }

        // touching spikes (see kSpikeTouchReach) also hurts
        Rect touch{Point2D(box.topLeft.x - 1.0f, box.topLeft.y - kSpikeTouchReach),
                   box.width + 2.0f, box.height + 2.0f * kSpikeTouchReach};
        for (const StaticBox& s : mStatic)
            if (s.damage && Overlaps(touch, s.box)) hurt = true;

        loc.x = box.topLeft.x - col.collisionBox.topLeft.x;
        loc.y = box.topLeft.y - col.collisionBox.topLeft.y;

        if (hurt && em.template HasComponent<RespawnComponent>(id)) {
            loc = em.template GetComponent<RespawnComponent>(id).loc;
            v = Point2D(0.0f, 0.0f);
            col.onGround = col.onCeiling = col.onLeftWall = col.onRightWall = 0;
            if (isPlayer) engine->GetSoundManager().GetSound(kHurtSound).play();
            return;
        }
        if (isPlayer && col.onGround && !wasOnGround)
            engine->GetSoundManager().GetSound(kLandSound).play();
        if (isPlayer && mShake) {
            float sx = (hitX > kShakeMinSpeed || hitX < -kShakeMinSpeed) ? hitX : 0.0f;
            float sy = (hitY > kShakeMinSpeed || hitY < -kShakeMinSpeed) ? hitY : 0.0f;
            if (sx != 0 || sy != 0) {
                mShake->offset = Point2D(sx * kShakeStrength, sy * kShakeStrength);
                mShake->frameLimit = kShakeFrames;
            }
        }
    }

    // Moving/collectable entities touching each other: the player collects stars, and stars
    // push apart and swap their speeds along the axis where they overlap the least.
    void ResolveDynamicPairs(EngineView<Components...>* engine) {
        auto& em = engine->GetEntityManager();
        mDynamic.clear();
        for (const Entity& e : em) {
            EntityID id = e.GetID();
            if (IsCollider(em, id) && !em.template GetComponent<CollisionComponent>(id).staticEntity)
                mDynamic.push_back(id);
        }

        for (size_t i = 0; i < mDynamic.size(); ++i) {
            for (size_t j = i + 1; j < mDynamic.size(); ++j) {
                EntityID a = mDynamic[i], b = mDynamic[j];
                if (!em.IsValid(a) || !em.IsValid(b)) continue;  // collected this frame
                Rect boxA = WorldBox(em, a), boxB = WorldBox(em, b);
                if (!Overlaps(boxA, boxB)) continue;

                bool playerA = em.template HasComponent<InputComponent>(a);
                bool playerB = em.template HasComponent<InputComponent>(b);
                if (playerA != playerB) {
                    Collect(engine, playerA ? b : a);
                } else if (!playerA) {
                    Separate(em, a, b, boxA, boxB);
                }
            }
        }
    }

    template <typename EM>
    static Rect WorldBox(EM& em, EntityID id) {
        return Offset(em.template GetComponent<CollisionComponent>(id).collisionBox,
                      em.template GetComponent<LocationComponent>(id).loc);
    }

    static void Collect(EngineView<Components...>* engine, EntityID item) {
        auto& em = engine->GetEntityManager();
        em.RemoveEntity(item);
        for (const Entity& e : em) {
            if (em.IsValid(e.GetID()) && em.template HasComponent<ScoreComponent>(e.GetID())) {
                ++em.template GetComponent<ScoreComponent>(e.GetID()).score;
                break;
            }
        }
        engine->GetSoundManager().GetSound(kStarSound).play();
    }

    template <typename EM>
    static void Separate(EM& em, EntityID a, EntityID b, const Rect& boxA, const Rect& boxB) {
        bool moveA = em.template HasComponent<MovementComponent>(a);
        bool moveB = em.template HasComponent<MovementComponent>(b);
        if (!moveA && !moveB) return;

        float overlapX = std::min(boxA.topLeft.x + boxA.width, boxB.topLeft.x + boxB.width) -
                         std::max(boxA.topLeft.x, boxB.topLeft.x);
        float overlapY = std::min(boxA.topLeft.y + boxA.height, boxB.topLeft.y + boxB.height) -
                         std::max(boxA.topLeft.y, boxB.topLeft.y);
        bool alongX = overlapX < overlapY;
        float push = alongX ? overlapX : overlapY;
        // which way to push a (away from b)
        float sign = alongX ? (boxA.topLeft.x < boxB.topLeft.x ? -1.0f : 1.0f)
                            : (boxA.topLeft.y < boxB.topLeft.y ? -1.0f : 1.0f);
        float shareA = (moveA && moveB) ? 0.5f : (moveA ? 1.0f : 0.0f);
        float shareB = 1.0f - shareA;

        Point2D& locA = em.template GetComponent<LocationComponent>(a).loc;
        Point2D& locB = em.template GetComponent<LocationComponent>(b).loc;
        if (alongX) {
            locA.x += sign * push * shareA;
            locB.x -= sign * push * shareB;
        } else {
            locA.y += sign * push * shareA;
            locB.y -= sign * push * shareB;
        }

        if (moveA && moveB) {
            Point2D& vA = em.template GetComponent<MovementComponent>(a).velocity;
            Point2D& vB = em.template GetComponent<MovementComponent>(b).velocity;
            if (alongX) {
                float t = vA.x;
                vA.x = vB.x * kBounce;
                vB.x = t * kBounce;
            } else {
                float t = vA.y;
                vA.y = vB.y * kBounce;
                vB.y = t * kBounce;
            }
        } else {
            Point2D& v = em.template GetComponent<MovementComponent>(moveA ? a : b).velocity;
            if (alongX) v.x = -v.x * kBounce;
            else v.y = -v.y * kBounce;
        }
    }

    std::vector<EntityID> mDynamic;  // moving/collectable entities this frame
    std::vector<StaticBox> mStatic;  // reused every frame to avoid reallocating
    CameraShake* mShake = nullptr;   // camera shake found this frame (valid only during Run)
};

}  // namespace ECSEngine
