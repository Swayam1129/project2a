#pragma once

#include "MathUtil.h"
#include "SpriteManager.h"

namespace ECSEngine {

// The sprite rectangle is relative to the entity location in the world
// This is only defined if an entity also has a LocationComponent.
class SpriteComponent {
public:
    SpriteID textureID;  // sprite from the SpriteManager
    Rect bounds;         // where to draw, relative to the LocationComponent; the image is
                         // scaled to fill this rect
    bool worldSpace;  // If false, this sprite is drawn in screen space instead of world space
private:
};

}  // namespace ECSEngine
