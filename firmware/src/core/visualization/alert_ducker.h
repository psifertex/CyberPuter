#pragma once
#include <cstdint>

namespace Visualization {
// UI-thread gain state only. Playback/SD ownership stays with the adapter.
// M5Unified squares channel volume: 30% of its setting is 9% amplitude (~-21 dB).
class AlertDucker {
public:
    uint8_t start(uint8_t currentMusicVolume) {
        if(!active)normal=currentMusicVolume;
        active=true;
        return uint8_t(unsigned(normal)*3/10);
    }
    uint8_t update(bool alertPlaying) {
        if(!alertPlaying)active=false;
        return active?uint8_t(unsigned(normal)*3/10):normal;
    }
    bool isActive() const { return active; }
    void reset() { active=false; }
private:
    uint8_t normal=0;
    bool active=false;
};
} // namespace Visualization
