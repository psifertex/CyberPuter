#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace Visualization {
// Single SD producer / single UI consumer. Speaker retains at most two blocks.
// The consumer must report occupancy before submitting more work, and markStop
// when requesting an asynchronous stop (which may discard the queued tail).
class PcmQueue {
public:
    static constexpr size_t SAMPLES=2048, COUNT=4;
    enum State : uint8_t { Free, Filling, Ready, Playing };
    struct Block {
        std::atomic<uint8_t> state{Free};
        uint32_t epoch=0, rate=0;
        size_t samples=0;
        int16_t pcm[SAMPLES]{};
    };
    // Producer only. Fill returned storage, then publish or cancel it.
    Block* acquire() {
        auto& block=blocks[producer];
        if(block.state.load()!=Free)return nullptr;
        block.state.store(Filling);return &block;
    }
    void publish() { blocks[producer].state.store(Ready);producer=(producer+1)%COUNT; }
    void cancel() { blocks[producer].state.store(Free); }
    // Consumer only. A stopped speaker can discard the SECOND queued pointer
    // before finishing the first: hold both until occupancy reaches zero.
    void markStop() { if(queuedCount)stopping=true; }
    bool reap(size_t occupied) {
        if(stopping){
            if(occupied)return false;
            for(size_t i=0;i<queuedCount;++i)blocks[submitted[i]].state.store(Free);
            queuedCount=0;stopping=false;
        }
        while(queuedCount>occupied){
            blocks[submitted[0]].state.store(Free);
            submitted[0]=submitted[1];--queuedCount;
        }
        return true;
    }
    // Discards obsolete prefetches while preserving ring order. The producer
    // never changes Ready blocks, so metadata is safe to inspect after acquire.
    Block* front(uint32_t epoch,bool enabled) {
        if(stopping)return nullptr;
        for(size_t i=0;i<COUNT;++i){
            auto& block=blocks[consumer];
            if(block.state.load()!=Ready)return nullptr;
            if(block.epoch==epoch && enabled)return &block;
            block.state.store(Free);consumer=(consumer+1)%COUNT;
        }
        return nullptr;
    }
    void submit() {
        blocks[consumer].state.store(Playing);
        submitted[queuedCount++]=uint8_t(consumer);consumer=(consumer+1)%COUNT;
    }
    size_t queued() const { return queuedCount; }
private:
    Block blocks[COUNT];
    size_t producer=0, consumer=0, queuedCount=0;
    uint8_t submitted[2]{};
    bool stopping=false;
};
} // namespace Visualization
