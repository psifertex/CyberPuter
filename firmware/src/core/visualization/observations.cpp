#include "observations.h"
#include <algorithm>
#include <cstdio>

namespace Visualization {
void ObservationStore::expire(uint32_t now) {
    for (auto& entry : entries)
        if (entry.used && uint32_t(now - entry.lastSeen) >= EXPIRE_MS) entry = {};
}

void ObservationStore::observe(const std::array<uint8_t, 7>& identity,
                               const char* name, size_t length, int rssi, uint32_t now) {
    expire(now);
    Observation* slot = nullptr;
    for (auto& entry : entries)
        if (entry.used && entry.identity == identity) { slot = &entry; break; }
    if (!slot) {
        for (auto& entry : entries) if (!entry.used) { slot = &entry; break; }
        // Full crowd: evict the least recently observed entry, never grow the table.
        if (!slot) {
            slot = &entries[0];
            for (auto& entry : entries)
                if (uint32_t(now - entry.lastSeen) > uint32_t(now - slot->lastSeen)) slot = &entry;
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
    if (name && length && name[0]) {
        size_t i = 0;
        for (; i < std::min(length, NAME_BYTES - 1) && name[i]; ++i) {
            const auto ch = static_cast<unsigned char>(name[i]);
            slot->name[i] = ch >= 32 && ch <= 126 ? char(ch) : '?';
        }
        slot->name[i] = '\0';
    }
    slot->lastSeen = now;
}
} // namespace Visualization
