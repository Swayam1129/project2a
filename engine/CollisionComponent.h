#pragma once

#include <cstdint>

#include "MathUtil.h"

namespace ECSEngine {

// Collision data. The flags are single bits to keep the component small.
// The on* flags are written by CollisionSystem each frame and read by Input/Gravity next frame.
struct CollisionComponent {
    uint8_t staticEntity : 1;  // 1 = never moves (tiles); 0 = moving or collectable
    uint8_t doesDamage : 1;    // 1 = touching it sends the player back to its respawn point
    uint8_t onGround : 1;      // standing on something (last frame)
    uint8_t onCeiling : 1;     // hit something above (last frame)
    uint8_t onLeftWall : 1;    // pushed against something on the left (last frame)
    uint8_t onRightWall : 1;   // pushed against something on the right (last frame)
    Rect collisionBox;          // [relative to LocationComponent] box used for collisions
    Rect previousCollisionBox;  // [world] the box at the start of the frame (CollisionLateSystem)
};

}  // namespace ECSEngine
