#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

// Advertisement heuristics, NOT authenticated identity or maliciousness verdicts.
// Primary-source identifiers and deliberate exclusions: docs/device-watchlist.md.
namespace DeviceClassifier {

enum class Platform : uint8_t {
    Unknown, Flipper, ChameleonUltra, ChameleonLite, PwnBeacon,
    AppleFindMy, GoogleFindHub, Tile, SamsungSmartTag,
    Meshtastic, MeshCore, Tesla, Omi, FlockBattery
};
enum class Category : uint8_t { Unknown, ResearchTool, Tracker, Mesh, Vehicle, Wearable, Surveillance };
enum class Evidence : uint8_t { None, Name, Service, Payload };

struct Match {
    Platform platform = Platform::Unknown;
    Evidence evidence = Evidence::None;
};
static_assert(sizeof(Match) == 2, "Observation classification must remain compact");

Match classifyName(std::string_view name);
Match classifyService(std::string_view uuid);
// Service data excludes its UUID. Manufacturer data INCLUDES the little-endian company ID.
Match classifyServiceData(std::string_view uuid, const uint8_t* data, size_t length);
Match classifyManufacturer(const uint8_t* data, size_t length);
// Stable, order-independent choice: structured evidence first, then watchlist categories.
Match strongerMatch(Match a, Match b);
Category category(Platform platform);
const char* label(Platform platform);
// A watchlist highlight means 'inspect this advertised capability', never 'attacker'.
bool isFlagged(Platform platform, bool includeFindMy = false);
inline bool isFlagged(Match match, bool includeFindMy = false) {
    return match.evidence != Evidence::None && isFlagged(match.platform, includeFindMy);
}

} // namespace DeviceClassifier
