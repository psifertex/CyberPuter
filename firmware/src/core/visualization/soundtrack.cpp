#include "soundtrack.h"
#include <initializer_list>

namespace Visualization {
void Soundtrack::setMusic(bool enabled, uint32_t, SoundSink& sink) {
    music=enabled;
    if(!enabled)for(auto voice:{Voice::Bass,Voice::Lead,Voice::Percussion})sink.stop(voice);
}
void Soundtrack::setEffects(bool enabled, SoundSink& sink) {
    effects=enabled;pending=pendingNamed=effectTail=hasEffectTime=false;
    if(!enabled)sink.stop(Voice::Effect);
}
void Soundtrack::silence(SoundSink& sink) {
    setMusic(false,0,sink);setEffects(false,sink);
}
void Soundtrack::notify(bool nameResolved) {
    if(!effects)return;
    pending=true;pendingNamed=pendingNamed||nameResolved;
}
void Soundtrack::tick(uint32_t now, SoundSink& sink) {
    if(effectTail && uint32_t(now-lastEffect)>=65){
        sink.note(Voice::Effect,effectNamed?1319:784,65);effectTail=false;
    }
    // Crowds coalesce to at most one two-note event every 750 ms.
    if(effects && pending && (!hasEffectTime || uint32_t(now-lastEffect)>=750)){
        effectNamed=pendingNamed;pending=pendingNamed=false;
        sink.note(Voice::Effect,effectNamed?880:523,45);
        lastEffect=now;hasEffectTime=true;effectTail=true;
    }
}
} // namespace Visualization
