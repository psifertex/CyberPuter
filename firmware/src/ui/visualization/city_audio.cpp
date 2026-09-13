#include "city_audio.h"
#include "core/visualization/soundtrack.h"
#include "infrastructure/platform/hardware.h"
#include "ui/menu/menu_controller.h"
#include <algorithm>

namespace CityAudio {
namespace {
using namespace Visualization;
Soundtrack track;
bool acquired=false;
uint8_t level=64, previousVolume=0, previousChannels[4]{};
// Fixed waveform lives for the entire playback lifetime. No streaming buffers.
const uint8_t WAVE[]={128,137,146,155,164,173,182,191,200,191,182,173,164,155,146,137,
                      128,119,110,101,92,83,74,65,56,65,74,83,92,101,110,119};
struct SpeakerSink final : SoundSink {
    void note(Voice voice,uint16_t hz,uint16_t ms) override {
        if(acquired)M5.Speaker.tone(hz,ms,4+int(voice),true,WAVE,sizeof(WAVE));
    }
    void stop(Voice voice) override { if(acquired)M5.Speaker.stop(4+int(voice)); }
} sink;
void release() {
    if(!acquired)return;
    for(uint8_t i=0;i<4;++i){M5.Speaker.stop(i+4);M5.Speaker.setChannelVolume(i+4,previousChannels[i]);}
    M5.Speaker.setVolume(previousVolume);acquired=false;
}
} // namespace
void begin() { track=Soundtrack{};level=std::min<uint8_t>(MenuController::getAlarmVolume(),64); }
void end() { track.silence(sink);release(); }
bool musicEnabled() { return track.musicEnabled(); }
bool effectsEnabled() { return track.effectsEnabled(); }
uint8_t volume() { return level; }
void toggleMusic(uint32_t now) {
    if(!musicEnabled() && !MenuController::getAudioEnabled())return;
    track.setMusic(!musicEnabled(),now,sink);
}
void toggleEffects() {
    if(!effectsEnabled() && !MenuController::getAudioEnabled())return;
    track.setEffects(!effectsEnabled(),sink);
}
void mute() { track.silence(sink);release(); }
void adjustVolume(int delta) {
    level=uint8_t(std::max(0,std::min(160,int(level)+delta)));
    if(acquired)M5.Speaker.setVolume(level);
}
void notify(bool nameResolved) { track.notify(nameResolved); }
void update(uint32_t now,bool scannerReady) {
    if(!MenuController::getAudioEnabled()){mute();return;}
    if(!musicEnabled()&&!effectsEnabled()){release();return;}
    // Let the legacy scan's final alert finish before borrowing the speaker.
    if(!scannerReady)return;
    if(!acquired){
        if(M5.Speaker.isPlaying())return;
        previousVolume=M5.Speaker.getVolume();
        for(uint8_t i=0;i<4;++i)previousChannels[i]=M5.Speaker.getChannelVolume(i+4);
        M5.Speaker.setVolume(level);
        M5.Speaker.setChannelVolume(4,160);M5.Speaker.setChannelVolume(5,80);
        M5.Speaker.setChannelVolume(6,64);M5.Speaker.setChannelVolume(7,120);
        acquired=true;
    }
    track.tick(now,sink);
}
} // namespace CityAudio
