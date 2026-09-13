#pragma once
#include <cstdint>
namespace CityAudio {
void begin();
void end();
void toggleMusic(uint32_t now);
void nextTrack();
const char* trackName();
const char* musicStatus();
void toggleEffects();
void mute();
void adjustVolume(int delta);
void notify(bool nameResolved);
void update(uint32_t now, bool scannerReady);
bool musicEnabled();
bool effectsEnabled();
uint8_t volume();
} // namespace CityAudio
