#include "observations.h"
#include <algorithm>
#include <cstdio>

namespace Visualization {
void ObservationStore::expire(uint32_t now) {
    for (auto& entry : entries)
        if (entry.used && uint32_t(now - entry.lastSeen) >= EXPIRE_MS) entry = {};
}

ObservationEvent ObservationStore::observe(const std::array<uint8_t, 7>& identity,
                               const char* name, size_t length, int rssi, uint32_t now, DeviceClassifier::Match classification) {
    expire(now);
    Observation* slot = nullptr;
    for (auto& entry : entries)
        if (entry.used && entry.identity == identity) { slot = &entry; break; }
    const bool incomingNamed = name && length && name[0];
    const auto rank=[](bool named,DeviceClassifier::Match match){return DeviceClassifier::isFlagged(match)?2:named?1:0;};
    const int incomingRank=rank(incomingNamed,classification);
    const bool isNew = slot == nullptr;
    const bool previouslyNamed = slot && slot->named;
    const bool previouslyFlagged = slot && DeviceClassifier::isFlagged(slot->classification);
    if (!slot) {
        for (auto& entry : entries) if (!entry.used) { slot = &entry; break; }
        // Full crowd: unknown traffic cannot evict a live named observation.
        // Within each class evict the oldest observation; never grow the table.
        if (!slot) {
            for (auto& entry : entries)
                if (rank(entry.named,entry.classification)<=incomingRank &&
                    (!slot || rank(entry.named,entry.classification)<rank(slot->named,slot->classification) ||
                     (rank(entry.named,entry.classification)==rank(slot->named,slot->classification) && uint32_t(now-entry.lastSeen)>uint32_t(now-slot->lastSeen)))) slot=&entry;
            if (!slot) return ObservationEvent::None;
        }
        *slot = {};
        slot->used = true;
        slot->identity = identity;
        slot->rssi = std::max(-127, std::min(0, rssi));
        slot->firstSeen = now;
        std::snprintf(slot->name, sizeof(slot->name), "ANON-%02X%02X", identity[0], identity[1]);
    } else {
        slot->rssi = (3 * slot->rssi + std::max(-127, std::min(0, rssi))) / 4;
    }
    // Empty scan responses must not erase an earlier advertised name. Sanitize
    // control bytes and unsupported UTF-8 bytes before rendering a tiny ASCII font.
    if (incomingNamed) {
        size_t i = 0;
        for (; i < std::min(length, NAME_BYTES - 1) && name[i]; ++i) {
            const auto ch = static_cast<unsigned char>(name[i]);
            slot->name[i] = ch >= 32 && ch <= 126 ? char(ch) : '?';
        }
        slot->name[i] = '\0';
        slot->named = true;
    }
    slot->lastSeen = now;
    slot->classification=DeviceClassifier::strongerMatch(slot->classification,classification);
    if(!previouslyFlagged && DeviceClassifier::isFlagged(slot->classification))return ObservationEvent::Flagged;
    if (slot->named && !previouslyNamed) return ObservationEvent::NameResolved;
    return isNew ? ObservationEvent::Discovered : ObservationEvent::None;
}

Selection selectPage(const Snapshot& entries, uint32_t now, size_t capacity,
                     uint32_t pageNumber) {
    Selection result;
    capacity=std::min(capacity,MAX_LABELS);
    std::array<uint8_t,MAX_OBSERVATIONS> ordered{};
    for(size_t i=0;i<entries.size();++i){
        const auto& e=entries[i];
        if(!e.used || uint32_t(now-e.lastSeen)>=EXPIRE_MS)continue;
        ordered[result.total++]=uint8_t(i);
        if(e.named)++result.named;
    }
    if(!capacity)return result;
    // Priority changes ordering, not page membership: no repeated/pinned rows.
    // Identity tie-breaks keep RSSI jitter and packet arrival order from shuffling.
    const auto rank=[](const Observation& e){
        return DeviceClassifier::isFlagged(e.classification)?2:e.named?1:0;
    };
    std::sort(ordered.begin(),ordered.begin()+result.total,[&](uint8_t a,uint8_t b){
        const int ar=rank(entries[a]),br=rank(entries[b]);
        return ar!=br?ar>br:entries[a].identity<entries[b].identity;
    });
    result.pages=std::max(size_t(1),(result.total+capacity-1)/capacity);
    result.page=pageNumber%result.pages;
    const size_t start=result.page*capacity;
    for(size_t i=start;i<std::min(start+capacity,result.total);++i)
        result.indices[result.count++]=ordered[i];
    return result;
}
} // namespace Visualization
