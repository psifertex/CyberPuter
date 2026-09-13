#include "renderer.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

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
void drawCity(Surface& s, const Frame& frame) {
    const uint32_t t=frame.now;
    s.fill(0,0,WIDTH,HEIGHT,Background);
    text(s,"GHOST DISTRICT",5,4,Violet);
    size_t count=0;for(const auto& e:frame.observations)if(e.used && uint32_t(t-e.lastSeen)<EXPIRE_MS)++count;
    char buffer[40];std::snprintf(buffer,sizeof(buffer),"BLE %02u",unsigned(count));text(s,buffer,199,4,Cyan);
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
    s.fill(88,37,59,24,Sign);box(s,88,37,59,24,Violet);
    text(s,"LABScon",97,41,White);text(s,"2026",106,52,Pink);
    // Six fixed sign locations, rotating through the bounded table every 8 s.
    // Stable table order avoids sign shuffling as RSSI fluctuates.
    static const int signs[][3]={{4,78,11},{155,70,12},{37,95,14},{151,101,13},{66,65,13},{3,49,12}};
    size_t visible=0;const size_t offset=count>6?(t/8000*6)%MAX_OBSERVATIONS:0;
    for(size_t n=0;n<MAX_OBSERVATIONS && visible<6;++n){
        const auto& e=frame.observations[(n+offset)%MAX_OBSERVATIONS];
        const uint32_t age=t-e.lastSeen;
        if(!e.used||age>=EXPIRE_MS)continue;
        const int x=signs[visible][0],y=signs[visible][1],maxChars=signs[visible][2];
        const size_t length=std::strlen(e.name),shown=std::min(length,size_t(maxChars));
        // Scroll long names inside the sign rather than drawing into its neighbour.
        const size_t scroll=length>shown?(t/400)%(length-shown+4):0;
        const size_t start=std::min(scroll,length-shown);
        char name[NAME_BYTES];std::memcpy(name,e.name+start,shown);name[shown]='\0';
        uint8_t color=visible%2?Pink:Cyan;
        if(e.rssi < -80)color=Amber;
        if(age>12000)color=visible%2?VioletDim:TealDim;
        if(uint32_t(t-e.firstSeen)<1200 && (t/150)%2)color=White;
        s.fill(x,y,int(shown)*6+7,13,Sign);box(s,x,y,int(shown)*6+7,13,color);text(s,name,x+4,y+3,color);
        ++visible;
    }
    for(int i=0;i<27;++i){const int x=(i*43+t/167)%240,y=18+(i*29+t/24)%99;
        for(int j=0;j<5;++j)pixel(s,x-j/2,std::min(119,y+j),Rain);}
    s.fill(0,113,240,8,Road);
    for(int i=0;i<30;++i)s.fill((i*37+t/125)%234,115+i%5,2+i%5,1,i%2?VioletDim:TealDim);
    s.fill(0,122,240,13,Sign);s.fill(0,121,240,1,Grid);
    if(frame.status)text(s,frame.status,5,125,Amber);
    else{std::snprintf(buffer,sizeof(buffer),"LABScon / %02u SIGNALS ALIVE",unsigned(count));text(s,buffer,5,125,Violet);}
    if(!count && !frame.status)text(s,"LISTENING...",78,102,Cyan);
}
const Renderer* rendererFor(Mode mode) {
    static const Renderer CITY{Mode::City,"Neon city",drawCity};
    switch(mode){
        case Mode::City:return &CITY;
        // TODO: Neon radar — share Surface/Snapshot; angles must be decorative.
        case Mode::Radar:return nullptr;
        // TODO: Signal rain — share Surface/Snapshot; profile frame time first.
        case Mode::Rain:return nullptr;
    }
    return nullptr;
}
} // namespace Visualization
