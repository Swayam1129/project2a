#pragma once

#include "MathUtil.h"

namespace ECSEngine {

// Where an entity goes back to when it touches something damaging (spikes).
struct RespawnComponent {
    Point2D loc;  // [world] top-left location to respawn at
};

}  // namespace ECSEngine
