#pragma once

#include "CameraFollowerComponent.h"
#include "CameraShakeComponent.h"
#include "MathUtil.h"

namespace ECSEngine {

// The camera. CameraSystem moves it to follow an entity and copies it into the WindowManager.
struct CameraComponent {
    Point2D position;          // [world] point shown at the window center (before shake)
    float worldUnitsPerPixel;  // zoom (1 = one world unit per pixel)
    Rect hardExclusion;  // [screen] the followed entity must stay inside this rect; outside it
                         // the camera snaps to keep it in (outer 10% of the width)
    Rect softExclusion;  // [screen] center region; outside it the camera eases toward the
                         // entity until the entity is back inside (center 40% of the width)
    float speed;         // easing fraction per frame (0..1); bigger = camera catches up faster
};

}  // namespace ECSEngine
