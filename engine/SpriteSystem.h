#pragma once

#include <string>

#include "CameraComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>

namespace ECSEngine {

// Draws every entity that has a SpriteComponent and a LocationComponent.
// World-space sprites are converted to window pixels through the WindowManager camera;
// screen-space sprites (e.g. the score) use their bounds directly as pixels.
// Sprites are scaled so the source image fills SpriteComponent::bounds.
// The engine calls display() after all systems have run.
template <typename... Components>
class SpriteSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name = "SpriteSystem";
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& sprites = engine->GetSpriteManager();
        auto& wm = engine->GetWindowManager();
        auto window = wm.GetWindow();

        for (const Entity& e : em) {
            EntityID id = e.GetID();
            if (!em.IsValid(id) || !em.template HasComponent<SpriteComponent>(id) ||
                !em.template HasComponent<LocationComponent>(id))
                continue;

            const SpriteComponent& sc = em.template GetComponent<SpriteComponent>(id);
            const Point2D& loc = em.template GetComponent<LocationComponent>(id).loc;
            Rect r{{loc.x + sc.bounds.topLeft.x, loc.y + sc.bounds.topLeft.y},
                   sc.bounds.width,
                   sc.bounds.height};
            if (sc.worldSpace) r = wm.WorldToWindow(r);

            sf::Sprite& sprite = sprites.GetSprite(sc.textureID);
            sf::Vector2i src = sprite.getTextureRect().size;
            sprite.setPosition({r.topLeft.x, r.topLeft.y});
            sprite.setScale({r.width / src.x, r.height / src.y});
            window->draw(sprite);
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
