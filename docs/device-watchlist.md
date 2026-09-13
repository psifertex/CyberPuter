# BLE platform watchlist

The device-list views highlight **watchlist signature matches**, not confirmed
attackers. A Bluetooth advertisement cannot authenticate a product, establish its
owner's intentions, or tell whether a tracker is following you. Names, service
UUIDs and even complete payloads can be copied. A conference will contain many
legitimate research tools. Do not use this costume firmware as an anti-stalking
or surveillance-safety system.

`DeviceClassifier` recognizes 13 platform/family entries. Eight are watchlist
entries by default (research tools, selected tracker protocols and the narrowly
matched Flock battery name); five are informational (Apple Find My, Meshtastic,
MeshCore, Tesla, Omi). Apple Find My can be opted into the watchlist, independently
of Google Find Hub, Tile and SmartTag; see the [keymap](keymap.md). The inferred
platform label is separate from an actual advertised name. For an unnamed device,
the views use `?PLATFORM`, without counting it as a discovered real name.

## Rules and primary sources

UUID comparisons are case-insensitive and accept
16-bit short, `0x`-prefixed, eight-digit, or full Bluetooth-base representations
where appropriate. Arbitrary 128-bit UUIDs are never shortened by substring.
Names are matched case-insensitively; suffixes below require a space, dash or
underscore boundary. These are deliberately conservative matches, not exhaustive
coverage of all firmware revisions.

| Platform / family | Advertisement evidence accepted | Meaning and source |
| --- | --- | --- |
| Flipper Zero | Services `3080`–`3083` | Research-tool watchlist. [Official serial profile](https://github.com/flipperdevices/flipperzero-firmware/blob/dev/targets/f7/ble_glue/profiles/serial_profile.c) ORs the hardware color into base `3080`. No inference about firmware or activity. |
| Chameleon Ultra | Exact `ChameleonUltra` name | Research-tool watchlist. [Official device-name constants](https://github.com/RfidResearchGroup/ChameleonUltra/blob/main/firmware/common/device_info.h) and [advertising setup](https://github.com/RfidResearchGroup/ChameleonUltra/blob/main/firmware/application/src/ble_main.c). |
| Chameleon Lite | Exact `ChameleonLite` name | Same sources as Ultra; name-only, not authenticated hardware identification. |
| PwnBeacon | `b34c0000-0000-0000-1337-000000000001` service | Research-tool/peer-beacon watchlist. [Implementing project's constants](https://github.com/pfefferle/palnagotchi/blob/main/palnagotchi/pwnbeacon.h). This is the BLE peer protocol, not proof of Pwnagotchi hardware or Wi-Fi activity. |
| Apple Find My format | Exactly 29 manufacturer bytes, including company `4c 00`, then type/length `12 19` | Informational by default; optionally included in the tracker-format watchlist. [OpenHaystack researchers' implementing firmware](https://github.com/seemoo-lab/openhaystack/blob/main/Firmware/ESP32/main/openhaystack_main.c). Could be a tag, other compatible accessory, Apple device or an emulator; never label all matches “AirTag.” |
| Google Find Hub format | Service-data UUID `FEAA`, frame `40`/`41`, and 21/22/33/34 payload bytes after UUID | Tracker-format watchlist. [Google specification, advertisement frames](https://developers.google.com/nearby/fast-pair/specifications/extensions/fmdn#advertising-frames). The lengths cover 20/32-byte ephemeral IDs and optional hashed flags. Compatible headphones can also emit these frames. Frame `41` does not independently prove a stalking incident. |
| Tile family | Service `FEED` | Tracker-family watchlist. [Original protocol/security research](https://www.usenix.org/system/files/conference/usenixsecurity26/sec26_prepub_kumar.pdf) documents the advertisements used for discovery. Could be a Tile-enabled accessory, not necessarily a separate tag. |
| Samsung SmartTag family | Service `FD59` or `FD5A` | Tracker-family watchlist. [Original SmartTag protocol research](https://www.usenix.org/system/files/usenixsecurity24-yu-tingfeng.pdf) documents unregistered/registered advertisements. Family match, not exact model or unwanted-tracking verdict. |
| Meshtastic | `6ba1b218-15a8-461f-9fa8-5dcae273eafd`, or `Meshtastic` name with optional bounded suffix | Informational mesh platform. [Official BLE interface](https://github.com/meshtastic/python/blob/master/meshtastic/ble_interface.py). A BLE observation does not capture LoRa traffic. |
| MeshCore | `MeshCore` name with optional bounded suffix | Informational, name-only mesh platform. [Official companion protocol](https://github.com/meshcore-dev/MeshCore/blob/main/docs/companion_protocol.md) describes this prefix. Its Nordic UART UUID is shared and is deliberately **not** sufficient. |
| Tesla BLE | Full service `00000211-b2d1-43f0-9b88-960cebf8b91e`, or `S` + 16 hex characters + `C` | Informational vehicle platform. [Tesla's protocol specification](https://github.com/teslamotors/vehicle-command/blob/main/pkg/protocol/protocol.md#ble). We do not connect to vehicles. |
| Omi | Exact `Omi` name | Informational, name-only wearable. [Official configuration](https://github.com/BasedHardware/omi/blob/main/omi/firmware/omi/omi.conf) and [advertisement setup](https://github.com/BasedHardware/omi/blob/main/omi/firmware/omi/src/lib/evt/ble.c). Not evidence that a microphone is recording. |
| Flock battery name | Exact `FS Ext Battery` | Surveillance watchlist, **name-only candidate**, not “confirmed camera.” [Field research project's observed BLE example](https://github.com/NSM-Barii/flock-back). Other Flock hardware and revisions may not advertise this name. |

## Highlighting and interpretation

Watchlist matches use amber labels, an exclamation marker and slowly pulsing
borders/markers. Ordinary discoveries do not blink. Informational matches have
no watchlist retention priority or alarm. Alert playback is described in the
[audio guide](soundtrack.md).

Apple Find My format matches do not reveal an owner-assigned item name, owner
identity, location history or authenticated model. The visualization does not
decode battery/class metadata. Rotating keys and addresses can produce multiple
observations for one physical device. Google frame 41 indicates protection mode,
not proof of stalking; its optional flags are not a general plaintext battery field.

Menu device-analysis tools use separate inherited decoders. Their model,
battery and state summaries are not validated by this visualization classifier.
The brief `FLAG <platform>` footer identifies a detection event, not confirmation
that an audio sample played.

## Excluded evidence

The `isTargetDevice` adapter uses the same name/service classifier as
the costume views. Generic ESP32 names, missing-name placeholders,
generic keyboard names, HC/RN module names and chip-vendor MAC prefixes are not
watchlist signals. Ordinary development boards and serial modules are not evidence of a
skimmer. `BruceNet` is a [default Wi-Fi AP SSID](https://github.com/pr3y/Bruce/wiki/bruce.conf),
not a reliable BLE platform identifier.

The inherited Xiao Biscuit compatibility helper retains only its explicit name,
not the common ESP32 example UUID. Nordic UART, standard Device Information/HID,
Arduino example services, Eddystone `FEAA` alone, Google Fast Pair `FE2C` alone,
and Apple company ID alone do not identify a watchlist platform.

The separate upstream Flock/Raven analysis module's scores are not used by the
costume views and do not validate this list. Wi-Fi-only surveillance platforms,
Classic-only tools and devices with their radios disabled cannot be covered by
adding names to a BLE scanner.

## Integration and tests

`firmware/src/core/detection/device_classifier.*` is allocation-free C++17 and
has no Arduino/NimBLE dependencies. `Match` is two bytes: a `Platform` and evidence
kind (`Name`, `Service`, `Payload`). Evidence is an ordering heuristic, not a
statistical confidence percentage. `strongerMatch` retains the strongest observed
evidence within an observation's lifetime and resolves equal-strength matches
deterministically; it does not accumulate fake confidence from repeated packets.

Feed all advertised service UUIDs, all service-data fields (without UUID bytes),
and all manufacturer fields (including little-endian company ID). Do this before
truncating/sanitizing names for display. No GATT connection, pairing, ringing,
tracking-key recovery, or active command is required. Normal BLE active scanning
only adds scan responses and cannot force a hidden identity to appear.

`tests/device_classifier_test.cpp` covers all platform signatures, bounded
prefixes, case/full UUID normalization, payload lengths/null pointers, missing
names and shared-service false positives. Run it through the root native CMake
test suite or directly with Clang C++17 and AddressSanitizer/UndefinedBehaviorSanitizer.

See [contributing](../CONTRIBUTING.md) for requirements when adding rules.
