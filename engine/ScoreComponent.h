#pragma once

#include "Entity.h"
#include "MathUtil.h"
#include "SpriteManager.h"

namespace ECSEngine {

// The game score. ScoreSystem shows it with three digit entities (hundreds, tens, ones).
class ScoreComponent {
public:
    int score;                    // points collected so far
    SpriteID digitTextureID[10];  // sprite for each digit 0..9
    EntityID digitID[3];          // entities drawing the hundreds, tens and ones digits

private:
};

}  // namespace ECSEngine
