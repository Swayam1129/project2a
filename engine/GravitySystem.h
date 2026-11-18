#pragma once

#include <string>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SoundManager.h"
#include "SoundNames.h"
#include "SystemManager.h"

namespace ECSEngine {

// Gravity tuning (pixels per frame, per frame).
const float kFallGravityMultiplier = 1.8f;      // fall faster than you rise
const float kFastFallGravityMultiplier = 1.6f;  // extra when holding Down
const float kMaxFallVelocity = 20.0f;
const float kMaxFastFallVelocity = 30.0f;
const float kWallSlideVelocity = 4.0f;          // max fall speed while pushing into a wall

// Adds acceleration to the velocity of every entity with Gravity and Movement.
// Uses last frame's collision flags and the player's keys:
//  - falling uses stronger gravity than rising, and holding Down makes it stronger still
//  - pushing into a wall while falling limits the fall speed (wall slide) and plays a sound
//  - GravityComponent::disabledTimer > 0 skips gravity for that many frames (used by dash)
template <typename... Components>
class GravitySystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("GravitySystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        for (const Entity& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id) || !em.template HasComponent<GravityComponent>(id) ||
                !em.template HasComponent<MovementComponent>(id))
                continue;

            auto& gravity = em.template GetComponent<GravityComponent>(id);
            Point2D& v = em.template GetComponent<MovementComponent>(id).velocity;
            if (gravity.disabledTimer > 0) {
                --gravity.disabledTimer;
                continue;
            }

            bool hasInput = em.template HasComponent<InputComponent>(id);
            bool downHeld = false, leftHeld = false, rightHeld = false;
            if (hasInput) {
                const auto& keys = em.template GetComponent<InputComponent>(id).keydown;
                downHeld = keys[static_cast<int>(sf::Keyboard::Scancode::Down)];
                leftHeld = keys[static_cast<int>(sf::Keyboard::Scancode::Left)];
                rightHeld = keys[static_cast<int>(sf::Keyboard::Scancode::Right)];
            }

            bool falling = v.y > 0;
            float ay = gravity.acceleration.y;
            if (falling) {
                ay *= kFallGravityMultiplier;
                if (downHeld) ay *= kFastFallGravityMultiplier;
            }
            v.x += gravity.acceleration.x;
            v.y += ay;

            float maxFall = downHeld ? kMaxFastFallVelocity : kMaxFallVelocity;
            if (hasInput && falling && em.template HasComponent<CollisionComponent>(id)) {
                const auto& col = em.template GetComponent<CollisionComponent>(id);
                bool wallSliding = (col.onLeftWall && leftHeld) || (col.onRightWall && rightHeld);
                if (wallSliding) {
                    maxFall = kWallSlideVelocity;
                    sf::Sound& sound = engine->GetSoundManager().GetSound(kWallSlideSound);
                    if (sound.getStatus() != sf::Sound::Status::Playing) sound.play();
                }
            }
            if (v.y > maxFall) v.y = maxFall;
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
