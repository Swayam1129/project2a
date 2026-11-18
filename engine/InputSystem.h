#pragma once

#include <algorithm>
#include <string>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SoundNames.h"
#include "SystemManager.h"

namespace ECSEngine {

// Player movement tuning (pixels per frame).
const float kMaxRunVelocity = 8.5f;
const float kGroundRunAcceleration = 1.5f;
const float kAirRunAcceleration = 1.0f;
const float kGroundFriction = 1.5f;      // slow-down when no arrow key is held, on the ground
const float kAirFriction = 0.4f;         // ... and in the air
const float kJumpVelocity = 22;
const float kJumpCutMultiplier = 0.4f;   // releasing jump early keeps this much upward speed
const float kWallJumpVelocityX = 10;
const float kWallJumpVelocityY = 20;
const float kDashVelocity = 30;
const float kDashSlowdown = 2.0f;        // how fast speed above kMaxRunVelocity decays after a dash
const int kDashFrames = 8;               // gravity is off for this many frames during a dash
const int kDashInterval = 60;            // frames between dashes (60 frames = 1 second)

// Turns the player's keys into velocity. Uses last frame's collision flags.
//   Left/Right : accelerate up to kMaxRunVelocity (works in the air too)
//   Z          : jump from the ground, or wall-jump away from a wall; releasing Z early
//                cuts the jump short (low jump vs. high jump)
//   C          : dash in the facing direction, at most once every kDashInterval frames
template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& sounds = engine->GetSoundManager();
        for (const Entity& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id) || !em.template HasComponent<InputComponent>(id) ||
                !em.template HasComponent<MovementComponent>(id))
                continue;

            auto& input = em.template GetComponent<InputComponent>(id);
            Point2D& v = em.template GetComponent<MovementComponent>(id).velocity;
            bool onGround = false, onLeftWall = false, onRightWall = false;
            if (em.template HasComponent<CollisionComponent>(id)) {
                const auto& col = em.template GetComponent<CollisionComponent>(id);
                onGround = col.onGround;
                onLeftWall = col.onLeftWall;
                onRightWall = col.onRightWall;
            }

            bool left = Held(input, sf::Keyboard::Scancode::Left);
            bool right = Held(input, sf::Keyboard::Scancode::Right);

            // --- running ---
            float accel = onGround ? kGroundRunAcceleration : kAirRunAcceleration;
            if (right && !left) {
                if (v.x < kMaxRunVelocity) v.x = std::min(v.x + accel, kMaxRunVelocity);
            } else if (left && !right) {
                if (v.x > -kMaxRunVelocity) v.x = std::max(v.x - accel, -kMaxRunVelocity);
            } else {
                float friction = onGround ? kGroundFriction : kAirFriction;
                v.x = (v.x > 0) ? std::max(v.x - friction, 0.0f) : std::min(v.x + friction, 0.0f);
            }
            if (v.x > kMaxRunVelocity) v.x = std::max(v.x - kDashSlowdown, kMaxRunVelocity);
            if (v.x < -kMaxRunVelocity) v.x = std::min(v.x + kDashSlowdown, -kMaxRunVelocity);

            // --- jumping ---
            if (Pressed(input, sf::Keyboard::Scancode::Z)) {
                if (onGround) {
                    v.y = -kJumpVelocity;
                    sounds.GetSound(kJumpSound).play();
                } else if (onLeftWall) {
                    v.y = -kWallJumpVelocityY;
                    v.x = kWallJumpVelocityX;
                    sounds.GetSound(kJumpSound).play();
                } else if (onRightWall) {
                    v.y = -kWallJumpVelocityY;
                    v.x = -kWallJumpVelocityX;
                    sounds.GetSound(kJumpSound).play();
                }
            }
            if (Released(input, sf::Keyboard::Scancode::Z) && v.y < 0) v.y *= kJumpCutMultiplier;

            // --- dashing ---
            if (input.lastDash < kDashInterval) ++input.lastDash;
            if (Pressed(input, sf::Keyboard::Scancode::C) && input.lastDash >= kDashInterval) {
                float dir = right ? 1.0f : left ? -1.0f : (v.x < 0 ? -1.0f : 1.0f);
                v.x = dir * kDashVelocity;
                v.y = 0;
                if (em.template HasComponent<GravityComponent>(id))
                    em.template GetComponent<GravityComponent>(id).disabledTimer = kDashFrames;
                input.lastDash = 0;
                sounds.GetSound(kDashSound).play();
            }
        }
        return kContinue;
    }

private:
    static bool Held(const InputComponent& in, sf::Keyboard::Scancode key) {
        return in.keydown[static_cast<int>(key)];
    }
    // true only on the frame the key goes down
    static bool Pressed(const InputComponent& in, sf::Keyboard::Scancode key) {
        int k = static_cast<int>(key);
        return in.keydown[k] && !in.lastKeydown[k];
    }
    // true only on the frame the key comes up
    static bool Released(const InputComponent& in, sf::Keyboard::Scancode key) {
        int k = static_cast<int>(key);
        return !in.keydown[k] && in.lastKeydown[k];
    }
};

}  // namespace ECSEngine
