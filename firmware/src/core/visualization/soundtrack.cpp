#include "soundtrack.h"
#include <initializer_list>

namespace Visualization {
void Soundtrack::setMusic(bool enabled, uint32_t now, SoundSink& sink) {
    music=enabled;step=0;lastStep=now-134;
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
    if(music && uint32_t(now-lastStep)>=134){
        // Skip missed beats after a slow frame; never enqueue a catch-up burst.
        const uint32_t advance=(now-lastStep)/134;
        lastStep=now;step=uint8_t((step+advance-1)%128);
        static const uint16_t bass[]={147,131,117,110};
        static const uint16_t chords[][4]={{294,349,440,587},{262,330,392,524},
                                          {233,294,349,466},{220,262,330,440}};
        static const uint8_t arp[]={0,2,1,3,2,1,0,2};
        const auto chord=step/32;
        if(step%2==0)sink.note(Voice::Bass,bass[chord],220);
        sink.note(Voice::Lead,chords[chord][arp[step%8]],90);
        if(step%4==0)sink.note(Voice::Percussion,82,45);
        else if(step%2)sink.note(Voice::Percussion,2489,12);
        step=(step+1)%128;
    }
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
