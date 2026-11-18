#pragma once

#include <bitset>

#include "Entity.h"
#include <SFML/Window/Keyboard.hpp>

namespace ECSEngine {

// Keyboard state for an entity the player controls. The engine fills keydown from SFML key
// events every frame (indexed by sf::Keyboard::Scancode) and copies the old keydown into
// lastKeydown first, so systems can tell "held" from "just pressed" / "just released".
struct InputComponent {
    InputComponent(EntityID whichEntity) : entity(whichEntity), lastDash(0) {}
    EntityID entity;  // the controlled entity
    int lastDash;     // frames since the last dash (InputSystem caps it at kDashInterval)
    std::bitset<sf::Keyboard::ScancodeCount> keydown;      // keys down this frame
    std::bitset<sf::Keyboard::ScancodeCount> lastKeydown;  // keys down last frame
};

}  // namespace ECSEngine
