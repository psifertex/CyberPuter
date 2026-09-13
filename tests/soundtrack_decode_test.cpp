#include "core/visualization/wav_stream.h"
#include "core/visualization/pcm_queue.h"
#include "core/visualization/alert_ducker.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

using namespace Visualization;
struct Memory final : WavReader {
    std::vector<uint8_t> data;
    uint32_t size() const override { return uint32_t(data.size()); }
    bool readAt(uint32_t offset,uint8_t* out,size_t bytes) override {
        if(offset>data.size() || bytes>data.size()-offset)return false;
        memcpy(out,data.data()+offset,bytes);return true;
    }
};
static void put32(std::vector<uint8_t>& data,size_t offset,uint32_t n) {
    for(unsigned i=0;i<4;++i)data[offset+i]=uint8_t(n>>(i*8));
}
static Memory sample() {
    Memory memory;
    memory.data={'R','I','F','F',38,0,0,0,'W','A','V','E',
        'f','m','t',' ',16,0,0,0,1,0,1,0,0x22,0x56,0,0,
        0x44,0xac,0,0,2,0,16,0,'d','a','t','a',2,0,0,0,1,0};
    return memory;
}
int main(int argc,char** argv) {
    AlertDucker duck;
    assert(!duck.isActive());
    assert(duck.start(160)==48 && duck.isActive());
    assert(duck.update(true)==48); // Queued/active alert keeps music quiet.
    assert(duck.start(48)==48); // Never compound attenuation on repeated starts.
    assert(duck.update(false)==160 && !duck.isActive()); // Finish or rejected play.
    assert(duck.start(120)==36); // Preserve a different music level.
    assert(duck.update(true)==36); // Async stop still has a retained sample.
    assert(duck.update(false)==120);
    duck.start(160);duck.reset();assert(!duck.isActive()); // Mute/exit drops ownership.
    assert(duck.start(80)==24 && duck.update(false)==80); // Fresh gain on re-entry.
    assert(duck.start(0)==0 && duck.update(false)==0); // Never unmute a silent channel.
    PcmQueue queue;
    auto* first=queue.acquire();assert(first);first->epoch=1;queue.publish();
    auto* second=queue.acquire();assert(second);second->epoch=1;queue.publish();
    assert(queue.front(1,true)==first);queue.submit();
    assert(queue.front(1,true)==second);queue.submit();
    for(unsigned i=0;i<2;++i){auto* b=queue.acquire();assert(b);b->epoch=1;queue.publish();}
    assert(!queue.acquire()); // Speaker + prefetch filled the bounded queue.
    queue.markStop();assert(!queue.reap(1)); // Stop discarded tail; head still in use.
    assert(!queue.acquire() && !queue.front(2,true)); // Neither pointer may be recycled.
    assert(queue.reap(0));assert(queue.queued()==0);
    assert(!queue.front(2,true)); // Old prefetches discarded after song switch.
    for(unsigned i=0;i<1000;++i){
        auto* b=queue.acquire();assert(b);b->epoch=2;b->pcm[0]=int16_t(i);queue.publish();
        assert(queue.front(2,true)==b && b->pcm[0]==int16_t(i));
        queue.submit();assert(queue.queued()==1);assert(queue.reap(0));
    }
    auto* cancelled=queue.acquire();assert(cancelled);queue.cancel();
    assert(queue.acquire()==cancelled);queue.cancel();
    WavInfo info;
    auto memory=sample();assert(readPcmWav(memory,info));
    assert(info.rate==22050 && info.offset==44 && info.bytes==2);
    for(size_t length=0;length<46;++length){
        auto broken=sample();broken.data.resize(length);assert(!readPcmWav(broken,info));
    }
    for(auto offset:{20,22,32,34}){
        auto broken=sample();broken.data[offset]++;assert(!readPcmWav(broken,info));
    }
    auto broken=sample();put32(broken.data,40,0xffffffff);assert(!readPcmWav(broken,info));
    broken=sample();put32(broken.data,4,0xffffffff);assert(!readPcmWav(broken,info));
    broken=sample();put32(broken.data,28,1);assert(!readPcmWav(broken,info));
    // Odd-size unknown metadata with RIFF padding before format is valid.
    memory=sample();memory.data.insert(memory.data.begin()+12,{'J','U','N','K',1,0,0,0,42,0});
    put32(memory.data,4,uint32_t(memory.data.size()-8));assert(readPcmWav(memory,info));
    assert(info.offset==54);
    for(int i=1;i<argc;++i){
        std::ifstream file(argv[i],std::ios::binary);assert(file.good());
        Memory bundled;bundled.data.assign(std::istreambuf_iterator<char>(file),{});
        assert(readPcmWav(bundled,info));
        if(info.rate==8000){
            assert(info.bytes>0 && info.bytes<=32000);
            int peak=0;
            for(size_t offset=info.offset;offset<info.offset+info.bytes;offset+=2){
                const int raw=int(bundled.data[offset])|(int(bundled.data[offset+1])<<8);
                const int value=raw>=32768?raw-65536:raw;
                peak=std::max(peak,value<0?-value:value);
            }
            assert(peak>=22500 && peak<=24000); // Approximately -3 dBFS, no clipping.
        }
        else assert(info.rate==22050 && info.bytes>22050*60);
        std::cout<<argv[i]<<": "<<double(info.bytes)/(info.rate*2)<<" seconds\n";
    }
    std::cout<<"PASS: alert ducking/restoration/peak; PCM ownership/async stop; WAV validation\n";
}
