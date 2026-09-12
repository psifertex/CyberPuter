#include "core/visualization/observations.h"
#include "core/visualization/renderer.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

using namespace Visualization;
static size_t count(const ObservationStore& store) {
    size_t n=0;for(const auto& e:store.snapshot())if(e.used)++n;return n;
}
struct Framebuffer final : Surface {
    uint8_t pixels[FRAME_BYTES]{};
    void fill(int x,int y,int w,int h,uint8_t color) override {
        assert(x>=0 && y>=0 && w>=0 && h>=0 && x+w<=WIDTH && y+h<=HEIGHT && color<16);
        for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx){
            const int index=yy*WIDTH+xx;
            auto& p=pixels[index/2];
            p=index%2?uint8_t((p&0xf0)|color):uint8_t((p&0x0f)|(color<<4));
        }
    }
    void save(const char* path){
        std::ofstream file(path,std::ios::binary);file<<"P6\n240 135\n255\n";
        for(int i=0;i<WIDTH*HEIGHT;++i){const uint8_t index=i%2?pixels[i/2]&15:pixels[i/2]>>4;
            const uint32_t c=PALETTE[index];const char rgb[]={char(c>>16),char(c>>8),char(c)};file.write(rgb,3);}
        assert(file.good());
    }
};
int main(int argc,char** argv){
    ObservationStore store;
    std::array<uint8_t,7> id{0x12,0x34,0,0,0,0,1};
    store.observe(id,"FIRST",5,-80,100);
    store.observe(id,"",0,-40,200);
    assert(count(store)==1 && std::strcmp(store.snapshot()[0].name,"FIRST")==0);
    assert(store.snapshot()[0].rssi==-70 && store.snapshot()[0].lastSeen==200);
    auto second=id;second[6]=0;
    store.observe(second,nullptr,0,-50,300);
    assert(count(store)==2); // Same address, different address type is distinct.
    assert(std::strcmp(store.snapshot()[1].name,"ANON-1234")==0);
    store.observe(second,"A\n\xFF" "B",4,-50,400);
    assert(std::strcmp(store.snapshot()[1].name,"A??B")==0);
    char longName[100];std::memset(longName,'X',sizeof(longName));
    store.observe(second,longName,sizeof(longName),-50,500);
    assert(std::strlen(store.snapshot()[1].name)==24);
    store.expire(20200);assert(count(store)==1);
    store.expire(20500);assert(count(store)==0);
    store.observe(id,"WRAP",4,-60,UINT32_MAX-500);
    store.expire(200);assert(count(store)==1);
    store.expire(EXPIRE_MS);assert(count(store)==0);
    for(int i=0;i<80;++i){id[0]=uint8_t(i);store.observe(id,"CROWD",5,-60,100+i);}
    assert(count(store)==MAX_OBSERVATIONS);
    bool latest=false;for(const auto& e:store.snapshot())if(e.identity[0]==79)latest=true;
    assert(latest);
    assert(rendererFor(Mode::City) && !rendererFor(Mode::Radar) && !rendererFor(Mode::Rain));
    Framebuffer frame;
    // Exercise every age/scroll phase, a full table, and uptime rollover.
    for(uint32_t t=200;t<25000;t+=137)drawCity(frame,{store.snapshot(),t,nullptr});
    drawCity(frame,{store.snapshot(),UINT32_MAX,nullptr});
    store.clear();drawCity(frame,{store.snapshot(),3000,nullptr});
    const char* names[]={"DECK-09","GHOST-7","NIGHT OWL","ANON-3F","PIXEL BUDS","NEON FOX"};
    for(int i=0;i<6;++i){id[0]=uint8_t(i);store.observe(id,names[i],std::strlen(names[i]),-43-i*7,1000);}
    drawCity(frame,{store.snapshot(),3000,nullptr});
    if(argc>1)frame.save(argv[1]);
    std::cout<<"PASS: observation bounds, identity, names, RSSI, expiry/wrap, mode lookup and pixel bounds\n";
    std::cout<<"Framebuffer "<<sizeof(frame.pixels)<<" bytes; table "<<sizeof(ObservationStore)<<" bytes\n";
}
