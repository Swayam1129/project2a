#include "SoundManager.h"

#include <cassert>
#include <iostream>

namespace ECSEngine {

void SoundManager::RegisterSound(const std::string& soundPath, const std::string& soundName) {
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(soundPath)) {
        std::cerr << "SoundManager: failed to load " << soundPath << "\n";
        return;
    }
    mSounds.erase(soundName);  // old sound points at the old buffer, so remove it first
    sf::SoundBuffer& stored = (mBuffers[soundName] = std::move(buffer));
    mSounds.emplace(soundName, sf::Sound(stored));
}

sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    auto it = mSounds.find(soundName);
    assert(it != mSounds.end());
    return it->second;
}

}  // namespace ECSEngine
