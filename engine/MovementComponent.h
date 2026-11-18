#pragma once

#include "MathUtil.h"

namespace ECSEngine {

// Velocity of an entity. MovementSystem adds it to the LocationComponent every frame.
struct MovementComponent {
    Point2D velocity;  // pixels per frame
};

}  // namespace ECSEngine
