#pragma once

#include "MathUtil.h"

namespace ECSEngine {

// Put on the camera entity: shake added on top of the camera position.
// While frameLimit > 0 the camera is pushed by offset, flipping direction every frame and
// shrinking, so the shake fades out. CollisionSystem starts a shake when the player hits a tile.
struct CameraShake {
    int frameLimit;  // frames of shake left
    Point2D offset;  // [world] current shake strength in x and y
};

}  // namespace ECSEngine
