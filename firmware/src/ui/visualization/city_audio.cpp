#include "city_audio.h"
#include "core/visualization/soundtrack.h"
#include "core/visualization/wav_stream.h"
#include "core/visualization/pcm_queue.h"
#include "infrastructure/platform/hardware.h"
#include "infrastructure/logging/logger.h"
#include "ui/menu/menu_controller.h"
#include <SD.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>

namespace CityAudio {
namespace {
using namespace Visualization;
Soundtrack track;
bool acquired=false;
uint8_t level=64, previousVolume=0, previousChannels[4]{};
constexpr uint8_t MUSIC_CHANNEL=4;
constexpr size_t MAX_TRACKS=16, NAME_BYTES=48;
constexpr const char* MUSIC_DIR="/cyberputer/music";
// Producer owns Filling, UI owns Ready/Playing. Neither reuses samples until
// M5Unified has released its retained pointer. Four buffers use 16 KB PCM.
PcmQueue pcmQueue;
std::atomic<uint32_t> request{1}, advances{0};
std::atomic<bool> wanted{false};
TaskHandle_t worker=nullptr;
portMUX_TYPE metadataLock=portMUX_INITIALIZER_UNLOCKED;
char displayName[NAME_BYTES]="SD PLAYLIST", displayStatus[32]="B: PLAY / N: NEXT";

void metadata(const char* name,const char* status) {
    portENTER_CRITICAL(&metadataLock);
    if(name)snprintf(displayName,sizeof(displayName),"%s",name);
    if(status)snprintf(displayStatus,sizeof(displayStatus),"%s",status);
    portEXIT_CRITICAL(&metadataLock);
}
struct FileReader final : WavReader {
    File& file;
    explicit FileReader(File& f):file(f){}
    uint32_t size() const override { return uint32_t(file.size()); }
    bool readAt(uint32_t offset,uint8_t* out,size_t bytes) override {
        return file.seek(offset) && file.read(out,bytes)==bytes;
    }
};
// All filesystem calls occur on this low-priority worker, never the UI thread.
// Logger/WiGLE SD access uses this existing mutex; skip busy periods without
// waiting. Arduino's filesystem/SPI locks serialize other legacy SD clients.
void musicWorker(void*) {
    std::array<std::array<char,NAME_BYTES>,MAX_TRACKS> names{};
    size_t count=0, selected=0;
    uint32_t localRequest=0, position=0;
    bool failed=false;
    File file;
    WavInfo format;
    for(;;){
        vTaskDelay(pdMS_TO_TICKS(5));
        const auto latest=request.load();
        if(latest==localRequest && (!wanted.load() || failed))continue;
        if(!logMutex || xSemaphoreTake(logMutex,0)!=pdTRUE)continue;
        if(latest!=localRequest){
            file.close();localRequest=latest;failed=false;position=0;
            const auto advance=advances.exchange(0);
            if(!count){
                if(SD.cardType()==CARD_NONE){metadata(nullptr,"NO SD - INSERT AND REBOOT");failed=true;}
                else{
                    File dir=SD.open(MUSIC_DIR);
                    if(dir && dir.isDirectory()){
                        for(unsigned entries=0;entries<256 && count<MAX_TRACKS;++entries){
                            File item=dir.openNextFile();
                            if(!item)break;
                            const char* name=item.name();
                            if(const char* slash=strrchr(name,'/'))name=slash+1;
                            const size_t len=strlen(name);
                            if(!item.isDirectory() && name[0]!='.' && len>4 && len<NAME_BYTES &&
                               !strcasecmp(name+len-4,".wav")){
                                snprintf(names[count++].data(),NAME_BYTES,"%s",name);
                            }
                            item.close();
                        }
                    }
                    dir.close();
                    std::sort(names.begin(),names.begin()+count,[](const auto& a,const auto& b){
                        return strcmp(a.data(),b.data())<0;
                    });
                    if(!count){metadata(nullptr,"NO WAV IN /cyberputer/music");failed=true;}
                }
            }
            if(count){selected=(selected+advance)%count;metadata(names[selected].data(),"READY");}
        }
        if(wanted.load() && !failed && count){
            if(file && position==format.bytes){
                file.close();selected=(selected+1)%count;position=0;
            }
            if(!file){
                char path[80];snprintf(path,sizeof(path),"%s/%s",MUSIC_DIR,names[selected].data());
                file=SD.open(path,FILE_READ);
                FileReader reader(file);
                if(!file || !readPcmWav(reader,format) || !file.seek(format.offset)){
                    file.close();failed=true;metadata(names[selected].data(),"BAD WAV - N: SKIP");
                }else metadata(names[selected].data(),"PLAYING SD");
            }
            auto* block=failed?nullptr:pcmQueue.acquire();
            if(block){
                const size_t bytes=std::min<size_t>(sizeof(block->pcm),format.bytes-position);
                const size_t got=file.read(reinterpret_cast<uint8_t*>(block->pcm),bytes);
                if(got!=bytes){
                    pcmQueue.cancel();file.close();failed=true;metadata(nullptr,"SD READ ERROR - N: RETRY");
                }else{
                    position+=bytes;block->epoch=localRequest;block->rate=format.rate;block->samples=bytes/2;
                    pcmQueue.publish();
                }
            }
        }
        xSemaphoreGive(logMutex);
    }
}
bool ensureWorker() {
    if(worker)return true;
    if(xTaskCreate(musicWorker,"CityMusic",4096,nullptr,1,&worker)!=pdPASS){
        worker=nullptr;metadata(nullptr,"NO RAM FOR MUSIC");return false;
    }
    return true;
}
void stopQueuedMusic() {
    if(acquired)M5.Speaker.stop(MUSIC_CHANNEL);
    // stop() posts an asynchronous marker and may discard the queued SECOND
    // block first. Retain both buffers until the channel becomes fully idle.
    pcmQueue.markStop();
}
void updateMusic() {
    const size_t occupied=pcmQueue.queued()?M5.Speaker.isPlaying(MUSIC_CHANNEL):0;
    if(!pcmQueue.reap(occupied))return;
    for(size_t attempts=0;attempts<PcmQueue::COUNT;++attempts){
        auto* block=pcmQueue.front(request.load(),wanted.load());
        if(!block || !acquired || pcmQueue.queued()==2)break;
        if(!M5.Speaker.playRaw(block->pcm,block->samples,block->rate,false,1,MUSIC_CHANNEL,false))break;
        pcmQueue.submit();
    }
}
const uint8_t WAVE[]={128,137,146,155,164,173,182,191,200,191,182,173,164,155,146,137,
                      128,119,110,101,92,83,74,65,56,65,74,83,92,101,110,119};
struct SpeakerSink final : SoundSink {
    void note(Voice voice,uint16_t hz,uint16_t ms) override {
        if(acquired)M5.Speaker.tone(hz,ms,4+int(voice),true,WAVE,sizeof(WAVE));
    }
    void stop(Voice voice) override { if(acquired)M5.Speaker.stop(4+int(voice)); }
} sink;
void release() {
    stopQueuedMusic();
    if(!acquired)return;
    for(uint8_t i=0;i<4;++i){M5.Speaker.stop(i+4);M5.Speaker.setChannelVolume(i+4,previousChannels[i]);}
    M5.Speaker.setVolume(previousVolume);acquired=false;
}
} // namespace
void begin() { track=Soundtrack{};wanted.store(false);++request;level=std::min<uint8_t>(MenuController::getAlarmVolume(),64); }
void end() { mute(); }
bool musicEnabled() { return track.musicEnabled(); }
bool effectsEnabled() { return track.effectsEnabled(); }
uint8_t volume() { return level; }
const char* trackName() {
    static char copy[NAME_BYTES];
    portENTER_CRITICAL(&metadataLock);memcpy(copy,displayName,sizeof(copy));portEXIT_CRITICAL(&metadataLock);
    return copy;
}
const char* musicStatus() {
    static char copy[32];
    portENTER_CRITICAL(&metadataLock);memcpy(copy,displayStatus,sizeof(copy));portEXIT_CRITICAL(&metadataLock);
    return copy;
}
void toggleMusic(uint32_t now) {
    if(!musicEnabled() && (!MenuController::getAudioEnabled() || !ensureWorker()))return;
    stopQueuedMusic();track.setMusic(!musicEnabled(),now,sink);
    wanted.store(musicEnabled());++request;
    if(!musicEnabled())metadata(nullptr,"MUSIC OFF");
}
void nextTrack() {
    if(!ensureWorker())return;
    stopQueuedMusic();++advances;++request;
}
void toggleEffects() {
    if(!effectsEnabled() && !MenuController::getAudioEnabled())return;
    track.setEffects(!effectsEnabled(),sink);
}
void mute() {
    if(wanted.exchange(false))++request;
    track.silence(sink);release();
}
void adjustVolume(int delta) {
    level=uint8_t(std::max(0,std::min(160,int(level)+delta)));
    if(acquired)M5.Speaker.setVolume(level);
}
void notify(bool nameResolved) { track.notify(nameResolved); }
void update(uint32_t now,bool scannerReady) {
    if(!MenuController::getAudioEnabled()){mute();updateMusic();return;}
    if(!musicEnabled()&&!effectsEnabled()){release();updateMusic();return;}
    if(!scannerReady){updateMusic();return;}
    if(!acquired){
        if(M5.Speaker.isPlaying())return;
        previousVolume=M5.Speaker.getVolume();
        for(uint8_t i=0;i<4;++i)previousChannels[i]=M5.Speaker.getChannelVolume(i+4);
        M5.Speaker.setVolume(level);
        M5.Speaker.setChannelVolume(4,160);M5.Speaker.setChannelVolume(5,80);
        M5.Speaker.setChannelVolume(6,64);M5.Speaker.setChannelVolume(7,120);
        acquired=true;
    }
    updateMusic();track.tick(now,sink);
}
} // namespace CityAudio
