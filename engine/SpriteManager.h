#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

using SpriteID = size_t;

// Owns all textures and sprites used by the game.
// Textures live in an unordered_map keyed by file path, so each file is loaded once and
// references to a texture never move. Sprites live in a vector; a SpriteID is an index into it.
class SpriteManager {
public:
    SpriteManager() = default;
    ~SpriteManager() = default;

    // Loads the texture at texturePath (once) and creates a sprite showing sourceRect of it.
    // Returns the new sprite's id. Ids stay valid for the lifetime of the SpriteManager.
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);

    // Returns the sprite for id. The reference is only valid until the next RegisterTexture call
    // (the vector may grow); store the SpriteID, not the reference.
    sf::Sprite& GetSprite(SpriteID id);

private:
    std::unordered_map<std::string, sf::Texture> mTextures;
    std::vector<sf::Sprite> mSprites;
};

}  // namespace ECSEngine
