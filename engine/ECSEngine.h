#pragma once

#include <optional>
#include <type_traits>
#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

template <typename... Components>
class EngineView {
public:
    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
};

template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine();

    void Run();

    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    WindowManager& GetWindowManager() override { return mWindowManager; }

    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

private:
    bool ProcessEvents();
    void DebugDraw();

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    WindowManager mWindowManager;

    bool running;  // false = paused (P)
    bool step;     // run exactly one frame while paused (N)
    int debug;     // non-zero = draw collision boxes (D)
};

template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name), running(true), step(false), debug(0) {}

template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {
    auto window = mWindowManager.GetWindow();
    if (window->isOpen()) window->close();
}

// Main game loop. Each frame:
//   1. ProcessEvents()  - read window/keyboard events and update every InputComponent
//   2. clear the window
//   3. run every system in the order they were added to the SystemManager
//      (SpriteSystem draws the entities during this step)
//   4. draw debug overlays (if enabled), then display() the finished frame
// The loop ends when the window is closed, Escape is pressed, or a system returns kExit.
// Debug keys: P = pause/resume, N = advance one frame while paused, D = toggle collision boxes.
template <typename... Components>
void ECSEngine<Components...>::Run() {
    auto window = mWindowManager.GetWindow();
    while (window->isOpen()) {
        if (!ProcessEvents()) {
            window->close();
            break;
        }
        if (!running && !step) {
            sf::sleep(sf::milliseconds(16));  // paused: don't burn CPU
            continue;
        }
        step = false;

        window->clear(sf::Color(208, 244, 247));
        if (!mSystemManager.Run(this)) {
            window->close();
            break;
        }
        if (debug) DebugDraw();
        window->display();
    }
}

// Reads all pending SFML events. Returns false when the game should quit.
// Key presses/releases are written into the keydown bitset of every entity that has an
// InputComponent. lastKeydown keeps the previous frame's keys so systems can detect the
// exact frame a key was pressed (keydown set, lastKeydown not set).
template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    constexpr bool kHasInput = (std::is_same_v<InputComponent, Components> || ...);

    if constexpr (kHasInput) {
        for (const Entity& e : mEntityManager) {
            if (mEntityManager.IsValid(e.GetID()) &&
                mEntityManager.template HasComponent<InputComponent>(e.GetID())) {
                auto& input = mEntityManager.template GetComponent<InputComponent>(e.GetID());
                input.lastKeydown = input.keydown;
            }
        }
    }

    auto window = mWindowManager.GetWindow();
    while (const std::optional event = window->pollEvent()) {
        if (event->template is<sf::Event::Closed>()) return false;

        bool pressed = false;
        sf::Keyboard::Scancode code = sf::Keyboard::Scancode::Unknown;
        if (const auto* key = event->template getIf<sf::Event::KeyPressed>()) {
            pressed = true;
            code = key->scancode;
        } else if (const auto* key = event->template getIf<sf::Event::KeyReleased>()) {
            code = key->scancode;
        } else {
            continue;
        }

        if (pressed) {
            if (code == sf::Keyboard::Scancode::Escape) return false;
            if (code == sf::Keyboard::Scancode::P) running = !running;
            if (code == sf::Keyboard::Scancode::N) step = true;
            if (code == sf::Keyboard::Scancode::D) debug = !debug;
        }

        int index = static_cast<int>(code);
        if (index < 0 || index >= static_cast<int>(sf::Keyboard::ScancodeCount)) continue;

        if constexpr (kHasInput) {
            for (const Entity& e : mEntityManager) {
                if (mEntityManager.IsValid(e.GetID()) &&
                    mEntityManager.template HasComponent<InputComponent>(e.GetID())) {
                    mEntityManager.template GetComponent<InputComponent>(e.GetID())
                        .keydown[index] = pressed;
                }
            }
        }
    }
    return true;
}

// Draws the collision box of every entity that has one (green = static, red = dynamic).
// Collision boxes are stored relative to the entity's LocationComponent.
template <typename... Components>
void ECSEngine<Components...>::DebugDraw() {
    constexpr bool kHasCollision = (std::is_same_v<CollisionComponent, Components> || ...);
    constexpr bool kHasLocation = (std::is_same_v<LocationComponent, Components> || ...);
    if constexpr (kHasCollision && kHasLocation) {
        for (const Entity& e : mEntityManager) {
            EntityID id = e.GetID();
            if (!mEntityManager.IsValid(id) ||
                !mEntityManager.template HasComponent<CollisionComponent>(id) ||
                !mEntityManager.template HasComponent<LocationComponent>(id))
                continue;
            const auto& col = mEntityManager.template GetComponent<CollisionComponent>(id);
            const auto& loc = mEntityManager.template GetComponent<LocationComponent>(id).loc;
            Rect box{Point2D(loc.x + col.collisionBox.topLeft.x, loc.y + col.collisionBox.topLeft.y),
                     col.collisionBox.width, col.collisionBox.height};
            if (col.staticEntity)
                mWindowManager.DebugFrameRect(box, 0, 200, 0);
            else
                mWindowManager.DebugFrameRect(box, 220, 0, 0);
        }
    }
}

}  // namespace ECSEngine
