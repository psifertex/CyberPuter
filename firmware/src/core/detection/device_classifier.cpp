#include "device_classifier.h"

namespace DeviceClassifier {
namespace {
char lower(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 'a' - 'A') : c; }
bool equal(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) if (lower(a[i]) != lower(b[i])) return false;
    return true;
}
bool prefix(std::string_view name, std::string_view stem) {
    if (name.size() < stem.size() || !equal(name.substr(0, stem.size()), stem)) return false;
    if (name.size() == stem.size()) return true;
    const char next = name[stem.size()];
    return next == ' ' || next == '_' || next == '-';
}
bool hex(char c) { c = lower(c); return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }
// Only shorten the Bluetooth base UUID, never arbitrary UUIDs sharing four characters.
std::string_view normalizeUuid(std::string_view uuid) {
    if (uuid.size() == 6 && equal(uuid.substr(0, 2), "0x")) uuid.remove_prefix(2);
    if (uuid.size() == 36 && equal(uuid.substr(0, 4), "0000") &&
        equal(uuid.substr(8), "-0000-1000-8000-00805f9b34fb")) uuid = uuid.substr(4, 4);
    if (uuid.size() == 8 && equal(uuid.substr(0, 4), "0000")) uuid = uuid.substr(4);
    return uuid;
}
struct ServiceRule { const char* uuid; Platform platform; };
constexpr ServiceRule SERVICE_RULES[] = {
    {"3080", Platform::Flipper}, {"3081", Platform::Flipper},
    {"3082", Platform::Flipper}, {"3083", Platform::Flipper},
    {"b34c0000-0000-0000-1337-000000000001", Platform::PwnBeacon},
    {"feed", Platform::Tile}, {"fd59", Platform::SamsungSmartTag}, {"fd5a", Platform::SamsungSmartTag},
    {"6ba1b218-15a8-461f-9fa8-5dcae273eafd", Platform::Meshtastic},
    {"00000211-b2d1-43f0-9b88-960cebf8b91e", Platform::Tesla},
};
struct NameRule { const char* name; Platform platform; bool allowSuffix; };
constexpr NameRule NAME_RULES[] = {
    {"ChameleonUltra", Platform::ChameleonUltra, false},
    {"ChameleonLite", Platform::ChameleonLite, false},
    {"Meshtastic", Platform::Meshtastic, true},
    {"MeshCore", Platform::MeshCore, true},
    {"Omi", Platform::Omi, false},
    {"FS Ext Battery", Platform::FlockBattery, false},
};
}

Match classifyName(std::string_view name) {
    // BLE names are bounded in the protocol; do not accept embedded NUL/truncation tricks.
    if (name.empty() || name.size() > 248 || name.find('\0') != std::string_view::npos) return {};
    for (const auto& rule : NAME_RULES) {
        if (rule.allowSuffix ? prefix(name, rule.name) : equal(name, rule.name))
            return {rule.platform, Evidence::Name};
    }
    // Tesla's documented legacy local name: S + 16 lowercase hex characters + C.
    if (name.size() == 18 && name.front() == 'S' && name.back() == 'C') {
        for (size_t i = 1; i < 17; ++i) if (!hex(name[i])) return {};
        return {Platform::Tesla, Evidence::Name};
    }
    return {};
}

Match classifyService(std::string_view uuid) {
    uuid = normalizeUuid(uuid);
    for (const auto& rule : SERVICE_RULES)
        if (equal(uuid, rule.uuid)) return {rule.platform, Evidence::Service};
    return {};
}

Match classifyServiceData(std::string_view uuid, const uint8_t* data, size_t length) {
    uuid = normalizeUuid(uuid);
    // FEAA is also Eddystone. FHN uses a distinct frame type and 20/32-byte EID.
    if (equal(uuid, "feaa") && data &&
        (length == 21 || length == 22 || length == 33 || length == 34) &&
        (data[0] == 0x40 || data[0] == 0x41)) return {Platform::GoogleFindHub, Evidence::Payload};
    return classifyService(uuid);
}

Match classifyManufacturer(const uint8_t* data, size_t length) {
    // Full Apple offline-finding frame: company 004c, type 12, length 19 (25 bytes).
    // This identifies Find My FORMAT, not AirTag hardware or unwanted tracking.
    if (data && length == 29 && data[0] == 0x4c && data[1] == 0x00 &&
        data[2] == 0x12 && data[3] == 0x19) return {Platform::AppleFindMy, Evidence::Payload};
    return {};
}

Match strongerMatch(Match a, Match b) {
    if (a.platform == Platform::Unknown) a = {};
    if (b.platform == Platform::Unknown) b = {};
    if (a.evidence != b.evidence) return a.evidence > b.evidence ? a : b;
    if (isFlagged(a) != isFlagged(b)) return isFlagged(a) ? a : b;
    return a.platform <= b.platform ? a : b;
}

Category category(Platform platform) {
    switch (platform) {
        case Platform::Flipper: case Platform::ChameleonUltra: case Platform::ChameleonLite:
        case Platform::PwnBeacon: return Category::ResearchTool;
        case Platform::AppleFindMy: case Platform::GoogleFindHub: case Platform::Tile:
        case Platform::SamsungSmartTag: return Category::Tracker;
        case Platform::Meshtastic: case Platform::MeshCore: return Category::Mesh;
        case Platform::Tesla: return Category::Vehicle;
        case Platform::Omi: return Category::Wearable;
        case Platform::FlockBattery: return Category::Surveillance;
        default: return Category::Unknown;
    }
}

bool isFlagged(Platform platform, bool includeFindMy) {
    if(platform == Platform::AppleFindMy)return includeFindMy;
    const Category c = category(platform);
    return c == Category::ResearchTool || c == Category::Tracker ||
           c == Category::Surveillance;
}

const char* label(Platform platform) {
    switch (platform) {
        case Platform::Flipper: return "FLIPPER";
        case Platform::ChameleonUltra: return "CHAMELEON ULTRA";
        case Platform::ChameleonLite: return "CHAMELEON LITE";
        case Platform::PwnBeacon: return "PWNBEACON";
        case Platform::AppleFindMy: return "FIND MY FORMAT";
        case Platform::GoogleFindHub: return "FIND HUB FORMAT";
        case Platform::Tile: return "TILE FAMILY";
        case Platform::SamsungSmartTag: return "SMARTTAG FAMILY";
        case Platform::Meshtastic: return "MESHTASTIC";
        case Platform::MeshCore: return "MESHCORE";
        case Platform::Tesla: return "TESLA BLE";
        case Platform::Omi: return "OMI NAME";
        case Platform::FlockBattery: return "FLOCK BATT NAME";
        default: return "UNKNOWN";
    }
}
} // namespace DeviceClassifier
