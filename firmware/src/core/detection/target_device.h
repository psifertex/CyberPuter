#pragma once

#include <Arduino.h>

// target_device.h
bool isTargetDevice(String name, String address, String serviceUuid, String deviceInfoService, String& outLabel);

// Compatibility wrappers. Advertisement-only matches are heuristic, not proof.
// Tesla: documented S + 16 hex chars + C name, or its full BLE service UUID.
bool isTeslaDevice(const String& name, const String& serviceUuid);

bool isXiaoBiscuitDevice(const String& name, const String& serviceUuid);

bool isFlipperDevice(const String& serviceUuid);
