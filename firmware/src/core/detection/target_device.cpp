#include "target_device.h"
#include "device_classifier.h"
#include "app/context/globals.h"
#include "infrastructure/logging/logger.h"

// Compatibility adapter: all product-watchlist matches remain unauthenticated.
bool isTargetDevice(String name, String address, String serviceUuid, String deviceInfoService, String& outLabel) {
    (void)address; // Chip-vendor OUIs do not identify a product or malicious use.
    (void)deviceInfoService;
    using namespace DeviceClassifier;
    const Match match = strongerMatch(classifyName({name.c_str(), name.length()}),
                                     classifyService({serviceUuid.c_str(), serviceUuid.length()}));
    outLabel = "";
    if (!isFlagged(match)) return false;
    outLabel = String(label(match.platform)) + " (heuristic)";
    LOG(LOG_TARGET, devTag + "Watchlist signature match, not a threat verdict: " + outLabel);
    return true;
}

bool isTeslaDevice(const String& name, const String& serviceUuid) {
    using namespace DeviceClassifier;
    return classifyName({name.c_str(), name.length()}).platform == Platform::Tesla ||
           classifyService({serviceUuid.c_str(), serviceUuid.length()}).platform == Platform::Tesla;
}

bool isXiaoBiscuitDevice(const String& name, const String& serviceUuid) {
    (void)serviceUuid;
    // Retain explicit-name recognition, not the widely shared ESP32 example UUID.
    return name == "Xiao Biscuit";
}

bool isFlipperDevice(const String& serviceUuid) {
    return DeviceClassifier::classifyService({serviceUuid.c_str(), serviceUuid.length()}).platform ==
           DeviceClassifier::Platform::Flipper;
}
