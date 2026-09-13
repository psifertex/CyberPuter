#include "core/visualization/observations.h"
#include "core/visualization/renderer.h"
#include "core/visualization/soundtrack.h"
#include "core/visualization/controls.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

using namespace Visualization;
struct Notes final : SoundSink {
    unsigned played=0;
    bool ready=true;
    std::vector<Voice> stopped;
    bool alert() override { if(!ready)return false;++played;return true; }
    void stop(Voice voice) override { stopped.push_back(voice); }
};
static void testSoundtrack() {
    Notes notes;Soundtrack track;
    track.notify(true);track.tick(1000,notes);assert(notes.played==0);
    track.setMusic(true,1000,notes);track.tick(1000,notes);
    assert(notes.played==0); // Music does not synthesize discovery chirps.
    track.silence(notes);assert(!track.musicEnabled()&&!track.effectsEnabled());
    assert(notes.stopped.size()==4);
    track.setEffects(true,notes);
    for(int i=0;i<100;++i){track.notify(false);track.tick(2000+i,notes);}
    assert(notes.played==0); // Ordinary events remain silent with F enabled.
    notes.ready=false;track.notify(true);track.tick(3000,notes);assert(notes.played==0);
    notes.ready=true;track.tick(3001,notes);assert(notes.played==1);
    for(int i=0;i<100;++i)track.notify(true);
    track.tick(8000,notes);assert(notes.played==1);
    track.tick(8001,notes);assert(notes.played==2);
    track.tick(50000,notes);assert(notes.played==2); // No repeat without a new flag.
    track.notify(true);track.setEffects(false,notes);track.tick(60000,notes);assert(notes.played==2);
    track.setEffects(true,notes);track.notify(true);track.tick(UINT32_MAX-50,notes);
    assert(notes.played==3);track.notify(true);track.tick(90,notes);assert(notes.played==3);
    track.tick(5000,notes);assert(notes.played==4); // Cooldown survives uptime wrap.
}
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
    testSoundtrack();
    assert(actionFor('p')==Action::Pause && actionFor('P')==Action::Pause);
    assert(actionFor('s')==Action::Stats && actionFor('S')==Action::Stats);
    assert(actionFor('m')==Action::Mute && actionFor('x')==Action::Mute);
    assert(actionFor('t')==Action::Findings && actionFor('h')==Action::Help);
    assert(actionFor(',')==Action::Left && actionFor('/')==Action::Right);
    assert(actionFor(';')==Action::Up && actionFor('.')==Action::Down);
    assert(actionFor('q')==Action::Menu && actionFor('`')==Action::Menu);
    for(const auto& b:BINDINGS)assert(actionFor(b.key,b.fn)==b.action);
    for(size_t i=0;i<helpLineCount();++i)assert(std::strlen(helpLine(i))<=38);
    ObservationStore store;
    std::array<uint8_t,7> id{0x12,0x34,0,0,0,0,1};
    store.observe(id,"FIRST",5,-80,100);
    store.observe(id,"",0,-40,200);
    assert(count(store)==1 && std::strcmp(store.snapshot()[0].name,"FIRST")==0);
    assert(store.snapshot()[0].rssi==-70 && store.snapshot()[0].lastSeen==200);
    auto second=id;second[6]=0;
    assert(store.observe(second,nullptr,0,-50,300)==ObservationEvent::Discovered);
    assert(count(store)==2); // Same address, different address type is distinct.
    assert(std::strcmp(store.snapshot()[1].name,"ANON-1234")==0);
    assert(store.observe(second,"A\n\xFF" "B",4,-50,400)==ObservationEvent::NameResolved);
    assert(store.snapshot()[1].named);
    assert(store.observe(second,"",0,-50,401)==ObservationEvent::None);
    assert(std::strcmp(store.snapshot()[1].name,"A??B")==0);
    char longName[100];std::memset(longName,'X',sizeof(longName));
    store.observe(second,longName,sizeof(longName),-50,500);
    assert(std::strlen(store.snapshot()[1].name)==24);
    store.expire(20200);assert(count(store)==1);
    store.expire(20500);assert(count(store)==0);
    store.observe(id,"WRAP",4,-60,UINT32_MAX-500);
    store.expire(200);assert(count(store)==1);
    store.expire(EXPIRE_MS);assert(count(store)==0);
    for(int i=0;i<200;++i){id[0]=uint8_t(i);store.observe(id,"CROWD",5,-60,100+i);}
    assert(count(store)==MAX_OBSERVATIONS);
    bool latest=false;for(const auto& e:store.snapshot())if(e.identity[0]==199)latest=true;
    assert(latest);
    assert(rendererFor(Mode::City) && rendererFor(Mode::Radar) && rendererFor(Mode::Rain));
    for(int i=0;i<100;++i){id[0]=uint8_t(i);store.observe(id,nullptr,0,-30,500);}
    for(const auto& e:store.snapshot())assert(e.named); // Anonymous traffic cannot evict names.
    auto selected=selectPage(store.snapshot(),500,12,0);
    assert(selected.count==12 && selected.named==96 && selected.pages==8);
    bool visited[MAX_OBSERVATIONS]{};
    for(unsigned page=0;page<8;++page){auto p=selectPage(store.snapshot(),500,12,page);
        for(size_t i=0;i<p.count;++i){assert(!visited[p.indices[i]]);visited[p.indices[i]]=true;}}
    id[0]=1;
    const DeviceClassifier::Match flagged{DeviceClassifier::Platform::Flipper,DeviceClassifier::Evidence::Service};
    assert(store.observe(id,nullptr,0,-60,501,flagged)==ObservationEvent::Flagged);
    selected=selectPage(store.snapshot(),501,12,0);
    assert(selected.named==95 && DeviceClassifier::isFlagged(store.snapshot()[selected.indices[0]].classification));
    assert(!store.snapshot()[selected.indices[0]].named);
    assert(store.observe(id,nullptr,0,-60,502,flagged)==ObservationEvent::None);
    assert(store.observe(id,"FLIPPER NAME",12,-60,503,flagged)==ObservationEvent::NameResolved);
    assert(DeviceClassifier::isFlagged(store.snapshot()[selected.indices[0]].classification));
    assert(selectPage(store.snapshot(),502,0,0).count==0);
    assert(selectPage(store.snapshot(),502,999,999).count<=MAX_LABELS);
    Framebuffer frame;
    for(size_t i=0;i<helpLineCount()+10;++i)drawHelp(frame,i);
    drawHelp(frame,SIZE_MAX);
    Snapshot empty{};Framebuffer background;
    for(auto mode:{Mode::City,Mode::Radar,Mode::Rain}){
        rendererFor(mode)->draw(frame,{store.snapshot(),1000,nullptr,5,false});
        rendererFor(mode)->draw(background,{empty,1000,nullptr,0,false});
        assert(std::memcmp(frame.pixels,background.pixels,FRAME_BYTES)==0);
    }
    // Exercise every age/scroll phase, a full table, and uptime rollover.
    for(auto mode:{Mode::City,Mode::Radar,Mode::Rain})
        for(uint32_t t=500;t<25000;t+=137)rendererFor(mode)->draw(frame,{store.snapshot(),t,nullptr,t/6000});
    drawCity(frame,{store.snapshot(),UINT32_MAX,nullptr});
    store.clear();drawCity(frame,{store.snapshot(),3000,nullptr});
    // Regression: rain used to repeat ten preferred rows and page only two
    // anonymous slots. Every observation must now occur on exactly one page.
    for(int namedCount:{0,2,8,10,12,20})for(size_t capacity:{6u,8u,12u}){
        store.clear();
        for(int i=0;i<24;++i){id[0]=uint8_t(i);store.observe(id,i<namedCount?"NAMED":nullptr,i<namedCount?5:0,-60,1000);}
        id[0]=31;store.observe(id,nullptr,0,-60,1000,flagged);
        const auto first=selectPage(store.snapshot(),1000,capacity,0);
        assert(first.total==25 && first.named==size_t(namedCount));
        assert(first.pages==(25+capacity-1)/capacity);
        assert(DeviceClassifier::isFlagged(store.snapshot()[first.indices[0]].classification));
        bool seen[MAX_OBSERVATIONS]{};size_t total=0;int previousRank=3;
        for(size_t page=0;page<first.pages;++page){
            const auto p=selectPage(store.snapshot(),1000,capacity,page);
            assert(p.count==std::min(capacity,size_t(25)-page*capacity));
            for(size_t n=0;n<p.count;++n){
                assert(!seen[p.indices[n]]);seen[p.indices[n]]=true;++total;
                const auto& e=store.snapshot()[p.indices[n]];
                const int rank=DeviceClassifier::isFlagged(e.classification)?2:e.named?1:0;
                assert(rank<=previousRank);previousRank=rank;
            }
        }
        assert(total==25);
        const auto wrap=selectPage(store.snapshot(),1000,capacity,first.pages);
        assert(wrap.indices==first.indices && wrap.page==0);
        // Reversed packet arrival and RSSI changes alone cannot shuffle pages.
        for(int i=23;i>=0;--i){id[0]=uint8_t(i);store.observe(id,nullptr,0,-30-i,1100);}
        assert(selectPage(store.snapshot(),1100,capacity,0).indices==first.indices);
        assert(selectPage(store.snapshot(),30000,capacity,100).count==0);
    }
    store.clear();
    const char* names[]={"DECK-09","GHOST-7","NIGHT OWL","ANON-3F","PIXEL BUDS","NEON FOX"};
    for(int i=0;i<6;++i){id[0]=uint8_t(i);store.observe(id,names[i],std::strlen(names[i]),-43-i*7,1000);}
    id[0]=10;store.observe(id,nullptr,0,-55,1000,flagged);
    id[0]=11;store.observe(id,"ChameleonUltra",14,-60,1000,DeviceClassifier::classifyName("ChameleonUltra"));
    drawCity(frame,{store.snapshot(),3000,nullptr});
    if(argc>1)frame.save(argv[1]);
    if(argc>2){drawRadar(frame,{store.snapshot(),3000,nullptr});frame.save(argv[2]);}
    if(argc>3){drawRain(frame,{store.snapshot(),3000,nullptr});frame.save(argv[3]);}
    if(argc>4){drawHelp(frame,7);frame.save(argv[4]);}
    std::cout<<"PASS: observations, flag transitions, suspicious-only alerts/cooldown/mute, expiry/wrap and pixel bounds\n";
    std::cout<<"Framebuffer "<<sizeof(frame.pixels)<<" bytes; table "<<sizeof(ObservationStore)<<" bytes\n";
}
