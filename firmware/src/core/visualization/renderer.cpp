#include "renderer.h"
#include "controls.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace Visualization {
const uint32_t PALETTE[16] = {0x03040b,0x0c0d1c,0x28233e,0x32304e,
    0x534169,0x386c70,0xb080ff,0x65ffe0,0xff59cc,0xe4fff9,0xffca73,
    0x31405a,0x131427,0x71548a,0x101123,0x000000};

// Five columns, seven rows. Font stays in flash; no runtime font allocation.
static const uint8_t FONT[][5] = {
 {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x42,0x61,0x51,0x49,0x46},
 {0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},
 {6,0x49,0x49,0x29,0x1e},
 {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
 {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,9,9,9,1},
 {0x3e,0x41,0x49,0x49,0x7a},{0x7f,8,8,8,0x7f},{0,0x41,0x7f,0x41,0},
 {0x20,0x40,0x41,0x3f,1},{0x7f,8,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
 {0x7f,2,0x0c,2,0x7f},{0x7f,4,8,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
 {0x7f,9,9,9,6},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,9,0x19,0x29,0x46},
 {0x46,0x49,0x49,0x49,0x31},{1,1,0x7f,1,1},{0x3f,0x40,0x40,0x40,0x3f},
 {0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,8,0x14,0x63},
 {7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43}
};
static void pixel(Surface& s, int x, int y, uint8_t c) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) s.fill(x,y,1,1,c);
}
static void text(Surface& s, const char* str, int x, int y, uint8_t color) {
    for (; *str && x < WIDTH; ++str, x += 6) {
        char ch = *str; uint8_t special[5]{};
        const uint8_t* glyph = special;
        // Preserve the mixed-case conference wordmark.
        if (ch == 'c') { special[0]=0x38;special[1]=special[2]=special[3]=0x44;special[4]=0x20; }
        else if (ch == 'o') { special[0]=special[4]=0x38;special[1]=special[2]=special[3]=0x44; }
        else if (ch == 'n') { special[0]=0x7c;special[1]=special[2]=special[3]=4;special[4]=0x78; }
        else {
            if (ch >= 'a' && ch <= 'z') ch -= 32;
            if (ch >= '0' && ch <= '9') glyph=FONT[ch-'0'];
            else if (ch >= 'A' && ch <= 'Z') glyph=FONT[10+ch-'A'];
            else if (ch == '-') std::memset(special,8,5);
            else if (ch == '=') std::memset(special,0x14,5);
            else if (ch == '+') {std::memset(special,8,5);special[2]=0x3e;}
            else if (ch == '/') {special[0]=0x40;special[1]=0x20;special[2]=8;special[3]=4;special[4]=2;}
            else if (ch == '.') special[2]=0x60;
            else if (ch == '!') special[2]=0x5f;
            else if (ch != ' ') {special[0]=2;special[1]=1;special[2]=0x51;special[3]=9;special[4]=6;}
        }
        for(int col=0;col<5;++col) for(int row=0;row<7;++row)
            if(glyph[col] & (1<<row)) pixel(s,x+col,y+row,color);
    }
}
static void box(Surface& s,int x,int y,int w,int h,uint8_t c) {
    s.fill(x,y,w,1,c);s.fill(x,y+h-1,w,1,c);s.fill(x,y,1,h,c);s.fill(x+w-1,y,1,h,c);
}
static void circle(Surface& s,int cx,int cy,int radius,uint8_t c) {
    int x=radius,y=0,e=1-radius;
    while(x>=y){
        pixel(s,cx+x,cy+y,c);pixel(s,cx+y,cy+x,c);pixel(s,cx-y,cy+x,c);pixel(s,cx-x,cy+y,c);
        pixel(s,cx-x,cy-y,c);pixel(s,cx-y,cy-x,c);pixel(s,cx+y,cy-x,c);pixel(s,cx+x,cy-y,c);
        ++y;if(e<0)e+=2*y+1;else{--x;e+=2*(y-x)+1;}
    }
}
static const char* displayName(const Observation& e,char (&buffer)[NAME_BYTES+2]) {
    if(!e.named && e.classification.evidence!=DeviceClassifier::Evidence::None){
        std::snprintf(buffer,sizeof(buffer),"?%s",DeviceClassifier::label(e.classification.platform));return buffer;
    }
    return e.name;
}
void drawCity(Surface& s, const Frame& frame) {
    const uint32_t t=frame.now;
    s.fill(0,0,WIDTH,HEIGHT,Background);
    text(s,"GHOST CITY",5,4,Violet);
    size_t count=0;for(const auto& e:frame.observations)if(frame.showFindings && e.used && uint32_t(t-e.lastSeen)<EXPIRE_MS)++count;
    const auto selection=selectPage(frame.observations,t,count>6?12:6,frame.page);
    char buffer[40];
    if(frame.showFindings){std::snprintf(buffer,sizeof(buffer),"N%u U%u %u/%u",unsigned(selection.named),unsigned(count-selection.named),unsigned(selection.page+1),unsigned(selection.pages));text(s,buffer,112,4,Cyan);}
    s.fill(4,15,232,1,Grid);
    for(int i=0;i<40;++i)pixel(s,(i*71)%240,20+(i*19)%75,i%4?Dim:Window);
    circle(s,196,40,15,VioletDim);circle(s,196,40,13,Dim);
    static const int buildings[][3]={{0,63,27},{29,46,29},{60,70,22},{84,31,66},{152,61,32},{187,52,28},{218,73,22}};
    for(size_t b=0;b<7;++b){
        const int x=buildings[b][0],y=buildings[b][1],w=buildings[b][2];
        s.fill(x,y,w,113-y,Building);s.fill(x,y,w,1,VioletDim);s.fill(x,y,1,113-y,Grid);
        for(int wx=x+4;wx<x+w-3;wx+=6)for(int wy=y+8;wy<110;wy+=8)
            s.fill(wx,wy,2,3,(wx+wy+b*7+t/3000)%7<2?(b%2?Window:TealDim):Dim);
    }
    s.fill(116,20,1,11,Violet);s.fill(115,19,3,2,Pink);
    const int brandY=count>6?22:37;
    s.fill(88,brandY,59,24,Sign);box(s,88,brandY,59,24,Violet);
    text(s,"LABScon",97,brandY+4,White);text(s,"2026",106,brandY+15,Pink);
    // Named devices lead each page; crowded scenes use twelve compact signs.
    static const int signs[][3]={{4,78,11},{155,70,12},{37,95,14},{151,101,13},{66,65,13},{3,49,12}};
    size_t visible=0;
    for(size_t n=0;frame.showFindings && n<selection.count;++n){
        const auto& e=frame.observations[selection.indices[n]];
        const uint32_t age=t-e.lastSeen;
        if(!e.used||age>=EXPIRE_MS)continue;
        const int x=count>6?4+int(visible%3)*80:signs[visible][0];
        const int y=count>6?48+int(visible/3)*16:signs[visible][1];
        const int maxChars=count>6?11:signs[visible][2];
        char derived[NAME_BYTES+2];const char* source=displayName(e,derived);
        const bool flagged=DeviceClassifier::isFlagged(e.classification);
        const size_t length=std::strlen(source),shown=std::min(length,size_t(maxChars-(flagged?1:0)));
        // Scroll long names inside the sign rather than drawing into its neighbour.
        const size_t scroll=length>shown?(t/400)%(length-shown+4):0;
        const size_t start=std::min(scroll,length-shown);
        char name[NAME_BYTES];std::memcpy(name,source+start,shown);name[shown]='\0';
        uint8_t color=visible%2?Pink:Cyan;
        if(age>12000)color=visible%2?VioletDim:TealDim;
        if(flagged)color=Amber;
        const int marker=flagged?6:0,w=int(shown)*6+7+marker;
        s.fill(x,y,w,13,Sign);box(s,x,y,w,13,flagged && (t/700)%2?VioletDim:color);
        if(flagged)text(s,"!",x+3,y+3,Amber);
        text(s,name,x+4+marker,y+3,color);
        ++visible;
    }
    for(int i=0;i<27;++i){const int x=(i*43+t/167)%240,y=18+(i*29+t/24)%99;
        for(int j=0;j<5;++j)pixel(s,x-j/2,std::min(119,y+j),Rain);}
    s.fill(0,113,240,8,Road);
    for(int i=0;i<30;++i)s.fill((i*37+t/125)%234,115+i%5,2+i%5,1,i%2?VioletDim:TealDim);
    s.fill(0,122,240,13,Sign);s.fill(0,121,240,1,Grid);
    if(frame.status)text(s,frame.status,5,125,Amber);
    else if(frame.showFindings){std::snprintf(buffer,sizeof(buffer),"LABScon / %02u SIGNALS ALIVE",unsigned(count));text(s,buffer,5,125,Violet);}
    if(frame.showFindings && !count && !frame.status)text(s,"LISTENING...",78,102,Cyan);
}
static void viewHeader(Surface& s,const char* title,const Selection& p,bool findings) {
    text(s,title,4,4,Violet);
    char b[30];std::snprintf(b,sizeof(b),"N%u U%u %u/%u",unsigned(p.named),unsigned(p.total-p.named),unsigned(p.page+1),unsigned(p.pages));
    if(findings)text(s,b,112,4,Cyan);
    s.fill(0,15,240,1,Grid);
}
static void viewFooter(Surface& s,const Frame& f) {
    s.fill(0,122,240,13,Sign);text(s,f.status?f.status:"1 CITY 2 RADAR 3 RAIN",4,125,Amber);
}
static void label(Surface& s,const Observation& e,int x,int y,size_t width,uint32_t now,uint8_t color) {
    const bool flagged=DeviceClassifier::isFlagged(e.classification);
    if(flagged){text(s,"!",x,y,(now/700)%2?VioletDim:Amber);x+=6;--width;color=Amber;}
    char derived[NAME_BYTES+2];const char* source=displayName(e,derived);
    char b[NAME_BYTES+2];const size_t len=std::strlen(source),n=std::min(len,width);
    const size_t start=len>n?std::min(size_t(now/400)%(len-n+4),len-n):0;
    std::memcpy(b,source+start,n);b[n]=0;text(s,b,x,y,color);
}
void drawRadar(Surface& s,const Frame& f) {
    s.fill(0,0,WIDTH,HEIGHT,Background);
    const auto p=selectPage(f.observations,f.now,8,f.page);viewHeader(s,"LABScon RADAR",p,f.showFindings);
    for(int r=15;r<=45;r+=15)circle(s,54,67,r,Grid);
    s.fill(9,67,91,1,Grid);s.fill(54,22,1,91,Grid);
    const float angle=(f.now%6000)*6.2831853f/6000;
    for(int r=0;r<46;++r)pixel(s,54+int(std::cos(angle)*r),67+int(std::sin(angle)*r),TealDim);
    for(const auto& e:f.observations){
        if(!f.showFindings||!e.used||uint32_t(f.now-e.lastSeen)>=EXPIRE_MS)continue;
        uint32_t hash=2166136261u;for(auto b:e.identity)hash=(hash^b)*16777619u;
        const float a=(hash%360)*0.017453293f;const int r=std::clamp((-int(e.rssi)-25)/2,5,44);
        const int x=54+int(std::cos(a)*r),y=67+int(std::sin(a)*r);
        const bool flagged=DeviceClassifier::isFlagged(e.classification);
        pixel(s,x,y,flagged?Amber:e.named?Cyan:VioletDim);
        if(flagged)circle(s,x,y,(f.now/700)%2?3:2,Amber);
        else if(e.named)circle(s,x,y,2,Cyan);
    }
    text(s,"ART NOT BEARING",9,114,VioletDim);
    for(size_t i=0;f.showFindings && i<p.count;++i)label(s,f.observations[p.indices[i]],112,23+int(i)*12,21,f.now,i%2?Pink:Cyan);
    viewFooter(s,f);
}
void drawRain(Surface& s,const Frame& f) {
    s.fill(0,0,WIDTH,HEIGHT,Background);
    const auto p=selectPage(f.observations,f.now,12,f.page);
    for(size_t i=0;i<(f.showFindings?p.count:12);++i){
        const char* glyphs=f.showFindings?f.observations[p.indices[i]].name:"0123456789ABCDEF";
        const size_t len=std::strlen(glyphs);
        for(int j=0;j<7 && len;++j){char ch[2]={glyphs[(f.now/240+j+i)%len],0};
            text(s,ch,8+int(i)*19,18+int((f.now/35+i*29+j*11)%99),j==0?TealDim:Dim);}
    }
    s.fill(0,0,240,17,Background);viewHeader(s,"SIGNAL RAIN",p,f.showFindings);
    s.fill(83,22,74,15,Sign);box(s,83,22,74,15,Pink);text(s,"LABScon",99,26,White);
    for(size_t i=0;f.showFindings && i<p.count;++i){int x=3+int(i%2)*120,y=44+int(i/2)*12;
        s.fill(x,y-1,115,10,Sign);label(s,f.observations[p.indices[i]],x+2,y,18,f.now,i%2?Pink:Cyan);}
    viewFooter(s,f);
}
void drawHelp(Surface& s,size_t scroll) {
    scroll=std::min(scroll,helpMaxScroll());
    s.fill(0,0,WIDTH,HEIGHT,Background);
    text(s,"LABScon / CONTROLS",5,4,Pink);s.fill(0,15,240,1,Grid);
    for(size_t i=0;i<HELP_ROWS && scroll+i<helpLineCount();++i)
        text(s,helpLine(scroll+i),5,21+int(i)*11,i%2?White:Cyan);
    s.fill(236,20,2,98,Grid);
    const int thumb=std::max(4,int(98*HELP_ROWS/helpLineCount()));
    const int y=20+int((98-thumb)*scroll/std::max(size_t(1),helpMaxScroll()));
    s.fill(236,y,2,thumb,Violet);
    s.fill(0,121,240,1,Grid);text(s,"UP/DOWN SCROLL  H/ESC CLOSE",4,125,Amber);
}
const Renderer* rendererFor(Mode mode) {
    static const Renderer CITY{Mode::City,"Neon city",drawCity};
    static const Renderer RADAR{Mode::Radar,"Neon radar",drawRadar};
    static const Renderer RAIN{Mode::Rain,"Signal rain",drawRain};
    switch(mode){
        case Mode::City:return &CITY;
        case Mode::Radar:return &RADAR;
        case Mode::Rain:return &RAIN;
    }
    return nullptr;
}
} // namespace Visualization
