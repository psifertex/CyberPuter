#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "core/detection/device_classifier.h"

namespace Visualization {
constexpr size_t MAX_OBSERVATIONS = 96;
constexpr size_t NAME_BYTES = 25;
constexpr uint32_t EXPIRE_MS = 20000;
struct Observation {
    std::array<uint8_t, 7> identity{}; // Address bytes plus BLE address type.
    char name[NAME_BYTES]{};
    int16_t rssi = -100;
    uint32_t lastSeen = 0;
    uint32_t firstSeen = 0;
    bool used = false;
    bool named = false;
    DeviceClassifier::Match classification{};
};
using Snapshot = std::array<Observation, MAX_OBSERVATIONS>;
enum class ObservationEvent { None, Discovered, NameResolved, Flagged };
constexpr size_t MAX_LABELS = 12;
struct Selection {
    std::array<uint8_t, MAX_LABELS> indices{};
    size_t count = 0, total = 0, named = 0, page = 0, pages = 1;
};
// Flagged observations and names stay ahead of ordinary anonymous traffic.
// Whole, non-overlapping pages; priority never pins a row on subsequent pages.
// Inferred platform labels never count as advertised names.
Selection selectPage(const Snapshot& entries, uint32_t now, size_t capacity,
                     uint32_t pageNumber);

// Platform-independent and allocation-free. The adapter owns synchronization.
class ObservationStore {
public:
    ObservationEvent observe(const std::array<uint8_t, 7>& identity, const char* name,
                 size_t length, int rssi, uint32_t now, DeviceClassifier::Match classification = {});
    void expire(uint32_t now);
    const Snapshot& snapshot() const { return entries; }
    void clear() { entries = {}; }
private:
    Snapshot entries{};
};
} // namespace Visualization
