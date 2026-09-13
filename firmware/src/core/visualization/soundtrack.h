#pragma once
#include <cstdint>

namespace Visualization {
enum class Voice : uint8_t { Bass, Lead, Percussion, Effect };
struct SoundSink {
    virtual ~SoundSink() = default;
    virtual void note(Voice voice, uint16_t hz, uint16_t durationMs) = 0;
    virtual void stop(Voice voice) = 0;
};
// Original procedural minor-key soundtrack. No samples, heap, waits or audio
// task. The platform speaker mixes four short, finite-duration voices.
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
    uint32_t lastStep = 0, lastEffect = 0;
    uint8_t step = 0;
};
} // namespace Visualization
