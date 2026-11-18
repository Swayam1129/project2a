#include "SpriteManager.h"

#include <cassert>
#include <iostream>

namespace ECSEngine {

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    auto it = mTextures.find(texturePath);
    if (it == mTextures.end()) {
        sf::Texture texture;
        if (!texture.loadFromFile(texturePath)) {
            std::cerr << "SpriteManager: failed to load " << texturePath << "\n";
        }
        it = mTextures.emplace(texturePath, std::move(texture)).first;
    }
    sf::IntRect rect({static_cast<int>(sourceRect.topLeft.x), static_cast<int>(sourceRect.topLeft.y)},
                     {static_cast<int>(sourceRect.width), static_cast<int>(sourceRect.height)});
    mSprites.emplace_back(it->second, rect);
    return mSprites.size() - 1;
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    assert(id < mSprites.size());
    return mSprites[id];
}

}  // namespace ECSEngine
