#pragma once

#include "MathUtil.h"

namespace ECSEngine {

// Acceleration applied to an entity's MovementComponent every frame by GravitySystem.
struct GravityComponent {
    GravityComponent(Point2D acceleration, int disabledTimer = 0)
        : acceleration(acceleration), disabledTimer(disabledTimer) {}
    Point2D acceleration;  // pixels per frame, per frame
    int disabledTimer;     // frames left with gravity switched off (set by the dash)
};

}  // namespace ECSEngine
