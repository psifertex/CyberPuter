#include "core/visualization/wav_stream.h"
#include "core/visualization/pcm_queue.h"
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
        if(info.rate==8000)assert(info.bytes>0 && info.bytes<=32000);
        else assert(info.rate==22050 && info.bytes>22050*60);
        std::cout<<argv[i]<<": "<<double(info.bytes)/(info.rate*2)<<" seconds\n";
    }
    std::cout<<"PASS: PCM queue ownership/async stop/epoch changes; WAV truncation/overflow/format/metadata\n";
}
