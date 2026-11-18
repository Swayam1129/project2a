#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include "CameraSystem.h"
#include "CollisionComponent.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "ECSEngine.h"
#include "EntityManager.h"
#include "GravitySystem.h"
#include "InputComponent.h"
#include "InputSystem.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementSystem.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SpawnComponent.h"
#include "SpawnSystem.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SoundNames.h"
#include "SpriteSystem.h"

std::string gResourcePath = "../assets/";
#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

// Map files from Canvas, placed in the assets folder.
const std::string kBackgroundMap = "sky.map";
const std::string kGameplayMap = "world.map";
const char kSpikeTile = 'x';
const ECSEngine::Point2D kPlayerStart(832.0f, 500.0f);
const float kGravity = 1.1f;
const float kCameraY = 512.0f;
const char kSpawnerTile = 'S';
const int kStarSpawnInterval = 90;  // frames between spawned stars (1.5 s)
const int kStarSpawnCount = 30;     // stars the spawner makes in total  // fixed camera height (world y at the window center)  // pixels per frame, per frame

// Sprite for a star (registered once, then reused).
ECSEngine::SpriteID StarSprite(Engine& engine) {
    static bool registered = false;
    static ECSEngine::SpriteID id = 0;
    if (!registered) {
        id = engine.GetSpriteManager().RegisterTexture(gResourcePath + "spritesheet-tiles-default.png",
                                                       {{640.0f, 320.0f}, 64.0f, 64.0f});
        registered = true;
    }
    return id;
}

// Turns a map entity into a star spawner.
void AddStarSpawner(Engine& engine, ECSEngine::EntityID spawner) {
    ECSEngine::SpawnComponent spawn;
    spawn.entityID = spawner;
    spawn.spawnType = "star";
    spawn.spawnTextureID = StarSprite(engine);
    spawn.timeBetweenSpawns = kStarSpawnInterval;
    spawn.timeToNextSpawn = kStarSpawnInterval;
    spawn.totalSpawns = kStarSpawnCount;
    engine.GetEntityManager().AddComponent(spawner, spawn);
}

// A star that sits still until collected (used to test wall jumping and dashing).
void PlaceStar(Engine& engine, const ECSEngine::Point2D& topLeft) {
    auto& em = engine.GetEntityManager();
    ECSEngine::EntityID star = em.CreateEntity("star");
    em.AddComponent(star, ECSEngine::LocationComponent{topLeft});
    em.AddComponent(star, ECSEngine::SpriteComponent{
                              StarSprite(engine), {{0.0f, 0.0f}, ECSEngine::kStarSize, ECSEngine::kStarSize}, true});
    ECSEngine::CollisionComponent collision{};
    collision.staticEntity = 0;
    collision.collisionBox = {{4.0f, 4.0f}, ECSEngine::kStarSize - 8.0f, ECSEngine::kStarSize - 8.0f};
    collision.previousCollisionBox = ECSEngine::Offset(collision.collisionBox, topLeft);
    em.AddComponent(star, collision);
}

// Creates the score display: a ScoreComponent plus three digit entities in the top-left
// corner of the screen (screen-space sprites, so they do not move with the camera).
void SetupScore(Engine& engine) {
    auto& em = engine.GetEntityManager();
    ECSEngine::ScoreComponent score;
    score.score = 0;
    for (int d = 0; d <= 9; ++d)  // digits are stacked in one column of the tile sheet
        score.digitTextureID[d] = engine.GetSpriteManager().RegisterTexture(
            gResourcePath + "spritesheet-tiles-default.png",
            {{832.0f, 832.0f - 64.0f * d}, 64.0f, 64.0f});
    for (int i = 0; i < 3; ++i) {
        ECSEngine::EntityID digit = em.CreateEntity("score digit");
        em.AddComponent(digit, ECSEngine::LocationComponent{{16.0f + i * 36.0f, 16.0f}});
        em.AddComponent(digit, ECSEngine::SpriteComponent{
                                   score.digitTextureID[0], {{0.0f, 0.0f}, 48.0f, 48.0f}, false});
        score.digitID[i] = digit;
    }
    ECSEngine::EntityID scoreEntity = em.CreateEntity("score");
    em.AddComponent(scoreEntity, score);
}

// Loads a map file (format described in the assignment) and creates one entity per map cell.
//   dictionary 1 : decoration only        -> Location + Sprite
//   dictionary 2 : solid / collectable    -> Location + Sprite + static Collision box
// The letter 'x' is a spike tile (doesDamage = 1). The letter 'S' becomes a star spawner.
// path is the folder holding the map and its sprite sheets, file is the map file name.
void LoadMap(Engine& engine, const std::string& path, const std::string& file) {
    std::ifstream in(path + file);
    if (!in) {
        std::cerr << "LoadMap: could not open " << path + file << "\n";
        return;
    }

    struct Tile {
        ECSEngine::SpriteID sprite;
        ECSEngine::Rect box;  // collision box relative to the tile (dictionary 2 only)
    };
    std::unordered_map<char, Tile> dictionary;

    std::string word;
    int dictType = 0;
    in >> word >> dictType;  // "dictionary" 1|2
    if (word != "dictionary" || (dictType != 1 && dictType != 2)) {
        std::cerr << "LoadMap: " << file << " does not start with 'dictionary 1|2'\n";
        return;
    }

    // Dictionary entries until the "map" line.
    while (in >> word && word != "map") {
        char letter = word[0];
        std::string sheet;
        float sx, sy, sw, sh;
        in >> sheet >> sx >> sy >> sw >> sh;
        Tile tile{};
        tile.sprite = engine.GetSpriteManager().RegisterTexture(path + sheet,
                                                                {{sx, sy}, sw, sh});
        if (dictType == 2) {
            float bx, by, bw, bh;
            in >> bx >> by >> bw >> bh;
            tile.box = {{bx, by}, bw, bh};
        }
        dictionary[letter] = tile;
    }

    // "map origin X Y tile W H size COLS ROWS"
    float originX, originY, tileW, tileH;
    int cols, rows;
    std::string originWord, tileWord, sizeWord;
    in >> originWord >> originX >> originY >> tileWord >> tileW >> tileH >> sizeWord >> cols >> rows;
    std::string line;
    std::getline(in, line);  // finish the "map ..." line

    auto& em = engine.GetEntityManager();
    for (int row = 0; row < rows && std::getline(in, line); ++row) {
        for (int col = 0; col < cols && col < static_cast<int>(line.size()); ++col) {
            auto it = dictionary.find(line[col]);
            if (it == dictionary.end()) continue;  // '.' or unknown letter = empty cell

            ECSEngine::EntityID id = em.CreateEntity(std::string(1, line[col]));
            em.AddComponent(id, ECSEngine::LocationComponent{
                                    {originX + col * tileW, originY + row * tileH}});
            em.AddComponent(id, ECSEngine::SpriteComponent{it->second.sprite,
                                                           {{0.0f, 0.0f}, tileW, tileH},
                                                           true});
            if (line[col] == kSpawnerTile) AddStarSpawner(engine, id);
            if (dictType == 2) {
                ECSEngine::CollisionComponent collision{};
                collision.staticEntity = 1;
                collision.doesDamage = (line[col] == kSpikeTile) ? 1 : 0;
                collision.collisionBox = it->second.box;
                collision.previousCollisionBox = it->second.box;
                em.AddComponent(id, collision);
            }
        }
    }
}

// Creates the player: a 64x64 alien that runs, jumps, collides and respawns at start.
ECSEngine::EntityID CreatePlayer(Engine& engine, const ECSEngine::Point2D& start) {
    auto& em = engine.GetEntityManager();
    ECSEngine::EntityID player = em.CreateEntity("player");
    ECSEngine::SpriteID sprite = engine.GetSpriteManager().RegisterTexture(
        gResourcePath + "spritesheet-characters-default.png", {{512.0f, 0.0f}, 128.0f, 128.0f});

    em.AddComponent(player, ECSEngine::LocationComponent{start});
    em.AddComponent(player, ECSEngine::MovementComponent{{0.0f, 0.0f}});
    em.AddComponent(player, ECSEngine::GravityComponent(ECSEngine::Point2D(0.0f, kGravity)));
    em.AddComponent(player, ECSEngine::SpriteComponent{sprite, {{0.0f, 0.0f}, 64.0f, 64.0f}, true});
    ECSEngine::CollisionComponent collision{};
    collision.staticEntity = 0;
    collision.collisionBox = {{14.0f, 16.0f}, 36.0f, 48.0f};  // body only, not the empty corners
    collision.previousCollisionBox = ECSEngine::Offset(collision.collisionBox, start);
    em.AddComponent(player, collision);
    ECSEngine::InputComponent input(player);
    input.lastDash = ECSEngine::kDashInterval;  // allow a dash right away
    em.AddComponent(player, input);
    em.AddComponent(player, ECSEngine::RespawnComponent{start});
    return player;
}

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    // Engine test code
    Engine engine(1024, 768, "Test Engine");

    // Load sounds
    auto& sounds = engine.GetSoundManager();
    sounds.RegisterSound(gResourcePath + "sfx_jump.ogg", ECSEngine::kJumpSound);
    sounds.RegisterSound(gResourcePath + "footstep_grass_003.ogg", ECSEngine::kLandSound);
    sounds.RegisterSound(gResourcePath + "footstep_snow_001.ogg", ECSEngine::kWallSlideSound);
    sounds.RegisterSound(gResourcePath + "sfx_jump-high.ogg", ECSEngine::kDashSound);
    sounds.RegisterSound(gResourcePath + "sfx_gem.ogg", ECSEngine::kStarSound);
    sounds.RegisterSound(gResourcePath + "error_008.ogg", ECSEngine::kHurtSound);

    // Load maps (background first so it is drawn behind the gameplay tiles)
    LoadMap(engine, gResourcePath, kBackgroundMap);
    LoadMap(engine, gResourcePath, kGameplayMap);

    // Place player
    ECSEngine::EntityID player = CreatePlayer(engine, kPlayerStart);

    // Fixed stars: one past the first spiky walls (needs wall jumps), one past the second
    // set (needs dashes)
    PlaceStar(engine, {2500.0f, 640.0f});
    PlaceStar(engine, {3290.0f, 640.0f});

    // Setup score component
    SetupScore(engine);

    // Setup Camera: follows the player in x, fixed height so the whole level height fits.
    {
        auto& em = engine.GetEntityManager();
        ECSEngine::EntityID camera = em.CreateEntity("camera");
        float w = engine.GetWindowManager().GetWindowBounds().width;
        float h = engine.GetWindowManager().GetWindowBounds().height;
        ECSEngine::CameraComponent cam{};
        cam.position = ECSEngine::Point2D(w / 2.0f, kCameraY);
        cam.worldUnitsPerPixel = 1.0f;
        cam.hardExclusion = {{0.1f * w, 0.0f}, 0.8f * w, h};  // never in the outer 10%
        cam.softExclusion = {{0.3f * w, 0.0f}, 0.4f * w, h};  // settle in the center 40%
        cam.speed = 0.06f;
        em.AddComponent(camera, cam);
        em.AddComponent(camera, ECSEngine::CameraFollower{player});
        em.AddComponent(camera, ECSEngine::CameraShake{0, {0.0f, 0.0f}});
    }

    // Add all systems
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());

    engine.Run();
    return 0;
}
