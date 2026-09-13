#pragma once
#include <cstdint>

namespace Visualization {
enum class Voice : uint8_t { Bass, Lead, Percussion, Effect };
struct SoundSink {
    virtual ~SoundSink() = default;
    virtual void note(Voice voice, uint16_t hz, uint16_t durationMs) = 0;
    virtual void stop(Voice voice) = 0;
};
// Audio toggles and crowd-rate-limited effects. Recorded music playback is
// handled by the platform's bounded PCM queue in CityAudio.
class Soundtrack {
public:
    void setMusic(bool enabled, uint32_t now, SoundSink& sink);
    void setEffects(bool enabled, SoundSink& sink);
    void silence(SoundSink& sink);
    void notify(bool nameResolved);
    void tick(uint32_t now, SoundSink& sink);
    bool musicEnabled() const { return music; }
    bool effectsEnabled() const { return effects; }
private:
    bool music = false, effects = false;
    bool pending = false, pendingNamed = false, effectTail = false;
    bool effectNamed = false, hasEffectTime = false;
    uint32_t lastEffect = 0;
};
} // namespace Visualization
