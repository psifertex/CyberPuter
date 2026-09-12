#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace Visualization {
constexpr size_t MAX_OBSERVATIONS = 24;
constexpr size_t NAME_BYTES = 25;
constexpr uint32_t EXPIRE_MS = 20000;
struct Observation {
    std::array<uint8_t, 7> identity{}; // Address bytes plus BLE address type.
    char name[NAME_BYTES]{};
    int16_t rssi = -100;
    uint32_t lastSeen = 0;
    uint32_t firstSeen = 0;
    bool used = false;
};
using Snapshot = std::array<Observation, MAX_OBSERVATIONS>;

// Platform-independent and allocation-free. The adapter owns synchronization.
class ObservationStore {
public:
    void observe(const std::array<uint8_t, 7>& identity, const char* name,
                 size_t length, int rssi, uint32_t now);
    void expire(uint32_t now);
    const Snapshot& snapshot() const { return entries; }
    void clear() { entries = {}; }
private:
    Snapshot entries{};
};
} // namespace Visualization
