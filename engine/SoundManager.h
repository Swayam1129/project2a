#pragma once

#include <string>
#include <unordered_map>

#include <SFML/Audio.hpp>

namespace ECSEngine {

// Owns all sound buffers and sounds, looked up by name.
// Both live in unordered_maps, so references to them never move while the manager exists.
class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager() = default;

    // Loads the sound file at soundPath and stores it under soundName.
    // Registering the same name again replaces the old sound.
    void RegisterSound(const std::string& soundPath, const std::string& soundName);

    // Returns the sound registered as soundName (asserts it exists). Call .play() on it.
    // The reference stays valid until that name is registered again or the manager is destroyed.
    sf::Sound& GetSound(const std::string& soundName);

private:
    std::unordered_map<std::string, sf::SoundBuffer> mBuffers;  // the audio data
    std::unordered_map<std::string, sf::Sound> mSounds;         // players that point at a buffer
};

}  // namespace ECSEngine
