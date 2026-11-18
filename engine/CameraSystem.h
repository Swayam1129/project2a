#pragma once

#include <string>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SystemManager.h"

namespace ECSEngine {

const float kShakeDecay = 0.8f;  // shake strength kept each frame

// Moves the camera to follow its CameraFollower target along x (y stays fixed):
//   - target outside softExclusion  -> camera eases toward it by `speed` each frame,
//     and keeps easing (even after the target stops) until it is back inside
//   - target outside hardExclusion  -> camera snaps so the target sits on the hard edge
// Then adds any CameraShake and sends the result to the WindowManager.
template <typename... Components>
class CameraSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name = "CameraSystem";
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& wm = engine->GetWindowManager();
        for (const Entity& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id) || !em.template HasComponent<CameraComponent>(id)) continue;
            auto& cam = em.template GetComponent<CameraComponent>(id);
            wm.SetWorldScale(cam.worldUnitsPerPixel);
            wm.SetWorldCenter(cam.position);

            if (em.template HasComponent<CameraFollower>(id)) {
                EntityID target = em.template GetComponent<CameraFollower>(id).entityID;
                if (em.IsValid(target) && em.template HasComponent<LocationComponent>(target))
                    Follow(engine, cam, target);
            }

            Point2D shown = cam.position;
            if (em.template HasComponent<CameraShake>(id)) {
                auto& shake = em.template GetComponent<CameraShake>(id);
                if (shake.frameLimit > 0) {
                    float sign = (shake.frameLimit % 2) ? 1.0f : -1.0f;
                    shown.x += sign * shake.offset.x;
                    shown.y += sign * shake.offset.y;
                    shake.offset = kShakeDecay * shake.offset;
                    --shake.frameLimit;
                }
            }
            wm.SetWorldCenter(shown);
        }
        return kContinue;
    }

private:
    // Screen-space x of the middle of the target (uses its collision box if it has one).
    static float TargetScreenX(EngineView<Components...>* engine, EntityID target) {
        auto& em = engine->GetEntityManager();
        Point2D loc = em.template GetComponent<LocationComponent>(target).loc;
        if (em.template HasComponent<CollisionComponent>(target)) {
            const Rect& box = em.template GetComponent<CollisionComponent>(target).collisionBox;
            loc.x += box.topLeft.x + box.width / 2.0f;
        }
        return engine->GetWindowManager().WorldToWindow(loc).x;
    }

    static void Follow(EngineView<Components...>* engine, CameraComponent& cam, EntityID target) {
        auto& wm = engine->GetWindowManager();

        // Soft zone: ease toward the target until it is back in the center region.
        float x = TargetScreenX(engine, target);
        float softLeft = cam.softExclusion.topLeft.x;
        float softRight = softLeft + cam.softExclusion.width;
        float error = 0;
        if (x < softLeft) error = x - softLeft;
        if (x > softRight) error = x - softRight;
        cam.position.x += wm.WindowToWorld(error) * cam.speed;
        wm.SetWorldCenter(cam.position);

        // Hard zone: never let the target reach the outer edges.
        x = TargetScreenX(engine, target);
        float hardLeft = cam.hardExclusion.topLeft.x;
        float hardRight = hardLeft + cam.hardExclusion.width;
        if (x < hardLeft) cam.position.x += wm.WindowToWorld(x - hardLeft);
        if (x > hardRight) cam.position.x += wm.WindowToWorld(x - hardRight);
        wm.SetWorldCenter(cam.position);
    }
};

}  // namespace ECSEngine
