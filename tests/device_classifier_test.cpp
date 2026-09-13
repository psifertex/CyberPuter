#include "core/detection/device_classifier.h"
#include <array>
#include <cassert>
#include <string>
using namespace DeviceClassifier;

int main() {
    assert(sizeof(Match) == 2);
    for (const char* uuid : {"3080", "3081", "3082", "3083", "0x3082", "0X3083", "00003081",
                            "00003082-0000-1000-8000-00805F9B34FB"})
        assert(classifyService(uuid).platform == Platform::Flipper);
    for (const char* uuid : {"abcd3082-0000-1000-8000-00805f9b34fb", "00003082-dead-beef-8000-00805f9b34fb", "3082junk"})
        assert(classifyService(uuid).platform == Platform::Unknown);
    assert(classifyService("b34c0000-0000-0000-1337-000000000001").platform == Platform::PwnBeacon);
    assert(classifyService("FEED").platform == Platform::Tile);
    assert(classifyService("fd59").platform == Platform::SamsungSmartTag);
    assert(classifyService("0000fd5a-0000-1000-8000-00805f9b34fb").platform == Platform::SamsungSmartTag);
    assert(classifyService("6ba1b218-15a8-461f-9fa8-5dcae273eafd").platform == Platform::Meshtastic);
    assert(classifyService("00000211-b2d1-43f0-9b88-960cebf8b91e").platform == Platform::Tesla);
    // Generic NUS, Arduino examples, SIG services and Fast Pair are NOT unique products.
    for (const char* uuid : {"6e400001-b5a3-f393-e0a9-e50e24dcca9e", "19b10000-e8f2-537e-4f6c-d104768a1214",
                            "4fafc201-1fb5-459e-8fcc-c5c9c331914b", "1812", "180a", "feaa", "fe2c"})
        assert(classifyService(uuid).platform == Platform::Unknown);

    assert(classifyName("ChameleonUltra").platform == Platform::ChameleonUltra);
    assert(classifyName("chameleonlite").platform == Platform::ChameleonLite);
    assert(classifyName("MeshCore_abcd").platform == Platform::MeshCore);
    assert(classifyName("Meshtastic 1234").platform == Platform::Meshtastic);
    assert(classifyName("Omi").platform == Platform::Omi);
    assert(classifyName("FS Ext Battery").platform == Platform::FlockBattery);
    assert(classifyName("S1a87a5a75f3df858C").platform == Platform::Tesla);
    for (const char* name : {"", "n/a", "<no name>", "ESP32", "HC-05", "RNBT-1234", "BruceNet", "Keyboard_a0",
                            "MeshCorey", "my ChameleonUltra", "Ominous", "penguin", "raven", "1234567890",
                            "S1a87a5a75f3df85gC", "S1a87a5a75f3df858X"})
        assert(classifyName(name).platform == Platform::Unknown);
    assert(classifyName(std::string("Omi\0spoof", 9)).platform == Platform::Unknown);
    assert(classifyName(std::string(249, 'x')).platform == Platform::Unknown);

    std::array<uint8_t, 40> data{};
    data[0] = 0x40;
    for (size_t size = 0; size <= data.size(); ++size) {
        const bool valid = size == 21 || size == 22 || size == 33 || size == 34;
        assert((classifyServiceData("feaa", data.data(), size).platform == Platform::GoogleFindHub) == valid);
    }
    data[0] = 0x41;
    assert(classifyServiceData("0000FEAA-0000-1000-8000-00805F9B34FB", data.data(), 22).platform == Platform::GoogleFindHub);
    assert(classifyServiceData("feaa", nullptr, 22).platform == Platform::Unknown);
    assert(classifyServiceData("fe2c", data.data(), 22).platform == Platform::Unknown);
    for (int frame : {0x00, 0x10, 0x20, 0x30, 0x42, 0xff}) {
        data[0] = static_cast<uint8_t>(frame);
        assert(classifyServiceData("feaa", data.data(), 22).platform == Platform::Unknown);
    }
    data[0] = 0x4c; data[1] = 0; data[2] = 0x12; data[3] = 0x19;
    for (size_t size = 0; size <= data.size(); ++size)
        assert((classifyManufacturer(data.data(), size).platform == Platform::AppleFindMy) == (size == 29));
    assert(classifyManufacturer(nullptr, 29).platform == Platform::Unknown);
    data[2] = 0x02; // iBeacon, not Find My.
    assert(classifyManufacturer(data.data(), 29).platform == Platform::Unknown);
    data[2] = 0x12; data[1] = 0x01;
    assert(classifyManufacturer(data.data(), 29).platform == Platform::Unknown);

    const Match name{Platform::Omi, Evidence::Name}, service{Platform::Tile, Evidence::Service};
    const Match payload{Platform::AppleFindMy, Evidence::Payload};
    assert(strongerMatch(name, service).platform == Platform::Tile);
    assert(strongerMatch(service, payload).platform == Platform::AppleFindMy);
    assert(strongerMatch(payload, {}).platform == Platform::AppleFindMy);
    for (int p = 0; p <= static_cast<int>(Platform::FlockBattery); ++p) {
        const Platform platform = static_cast<Platform>(p);
        assert(label(platform) != nullptr);
        for (int q = 0; q <= static_cast<int>(Platform::FlockBattery); ++q) {
            const Match a{platform, Evidence::Service}, b{static_cast<Platform>(q), Evidence::Service};
            assert(strongerMatch(a, b).platform == strongerMatch(b, a).platform);
        }
    }
    assert(!isFlagged(Platform::Meshtastic));
    assert(!isFlagged(Platform::MeshCore));
    assert(!isFlagged(Platform::Tesla));
    assert(!isFlagged(Platform::Omi));
    assert(isFlagged(Platform::Flipper));
    assert(!isFlagged(Platform::AppleFindMy));
    assert(isFlagged(Platform::AppleFindMy,true));
    assert(!isFlagged(payload) && isFlagged(payload,true));
    assert(isFlagged(Platform::GoogleFindHub)); // G is Apple-specific.
    assert(isFlagged(Platform::Tile) && isFlagged(Platform::SamsungSmartTag));
    assert(!isFlagged(Match{}));
    return 0;
}
