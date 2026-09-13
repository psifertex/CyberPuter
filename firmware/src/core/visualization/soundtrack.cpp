#include "soundtrack.h"
#include <initializer_list>

namespace Visualization {
void Soundtrack::setMusic(bool enabled, uint32_t, SoundSink& sink) {
    music=enabled;
    if(!enabled)for(auto voice:{Voice::Bass,Voice::Lead,Voice::Percussion})sink.stop(voice);
}
void Soundtrack::setEffects(bool enabled, SoundSink& sink) {
    effects=enabled;pending=hasEffectTime=false;
    if(!enabled)sink.stop(Voice::Effect);
}
void Soundtrack::silence(SoundSink& sink) {
    setMusic(false,0,sink);setEffects(false,sink);
}
void Soundtrack::notify(bool suspicious) {
    if(effects && suspicious)pending=true;
}
void Soundtrack::tick(uint32_t now, SoundSink& sink) {
    // Only newly flagged observations enter this path. Coalesce crowds and
    // retain events while the SD sample loads; never emit ordinary chirps.
    if(effects && pending && (!hasEffectTime || uint32_t(now-lastEffect)>=5000) && sink.alert()){
        pending=false;lastEffect=now;hasEffectTime=true;
    }
}
} // namespace Visualization
