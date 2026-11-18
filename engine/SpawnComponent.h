#pragma once

#include <string>

#include "Entity.h"
#include "MathUtil.h"
#include "SpriteManager.h"

namespace ECSEngine {

// Makes an entity a spawner. Every timeBetweenSpawns frames SpawnSystem creates a new
// entity of spawnType next to it, until totalSpawns reaches 0.
class SpawnComponent {
public:
    EntityID entityID;       // The entity that this spawn component is attached to
    std::string spawnType;   // what to spawn (currently only "star")
    SpriteID spawnTextureID; // sprite used by spawned entities
    int timeToNextSpawn;     // frames until the next spawn
    int timeBetweenSpawns;   // in frames
    int totalSpawns;         // spawns left

private:
};

}  // namespace ECSEngine
