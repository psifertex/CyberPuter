#include "wav_stream.h"
#include <cstring>

namespace Visualization {
namespace {
uint16_t le16(const uint8_t* p) { return uint16_t(p[0]) | uint16_t(p[1])<<8; }
uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1])<<8 |
                                      uint32_t(p[2])<<16 | uint32_t(p[3])<<24; }
}
bool readPcmWav(WavReader& reader, WavInfo& out) {
    out={};
    uint8_t header[16];
    if(reader.size()<44 || !reader.readAt(0,header,12) ||
       std::memcmp(header,"RIFF",4) || std::memcmp(header+8,"WAVE",4))return false;
    const uint32_t riffBytes=le32(header+4);
    if(riffBytes<36 || riffBytes>reader.size()-8)return false;
    const uint32_t end=riffBytes+8;
    bool formatFound=false;
    uint32_t rate=0;
    for(uint32_t offset=12,chunks=0;chunks<32 && offset<=end-8 && offset<65536;++chunks){
        if(!reader.readAt(offset,header,8))return false;
        const uint32_t bytes=le32(header+4), data=offset+8;
        if(bytes>end-data)return false;
        if(!std::memcmp(header,"fmt ",4)){
            if(formatFound || bytes<16 || !reader.readAt(data,header,16))return false;
            rate=le32(header+4);
            if(le16(header)!=1 || le16(header+2)!=1 || rate<8000 || rate>44100 ||
               le32(header+8)!=rate*2 || le16(header+12)!=2 || le16(header+14)!=16)return false;
            formatFound=true;
        }else if(!std::memcmp(header,"data",4)){
            if(!formatFound || bytes<2 || (bytes&1))return false;
            out={rate,data,bytes};return true;
        }
        const uint64_t next=uint64_t(data)+bytes+(bytes&1);
        if(next>end)return false;
        offset=uint32_t(next);
    }
    return false;
}
} // namespace Visualization
