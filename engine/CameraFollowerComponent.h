#pragma once

#include "Entity.h"

namespace ECSEngine {

// Put on the camera entity: which entity the camera follows.
// CameraSystem looks up the target's LocationComponent (and CollisionComponent, if any).
struct CameraFollower {
    EntityID entityID;  // The entity that this camera follows
};

}  // namespace ECSEngine
