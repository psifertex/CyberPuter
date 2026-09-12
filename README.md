# Cyberputer

Exploration of a cyberpunk radio visualization for the M5Stack Cardputer ADV.

## LABScon costume previews

Open `preview/index.html` directly in a browser. It needs no server, build or
network access. Three animated concepts use the exact 240×135 framebuffer:
neon radar, device-name rain, and a neon city. Controls provide 1×/2×/3× display
scales, pause, synthetic crowd density, and native PNG downloads. All names are
fictional. LABScon is custom pixel lettering, not the official logo artwork.

GhostBLE is the selected firmware base. Its existing bug is being investigated
in a separate session; the user's forthcoming patch will be integrated later.
The previews do not modify its scanning implementation or require that fix.

The local Git repository pins GhostBLE as a submodule. After cloning this
repository, run `git submodule update --init --recursive` to fetch that base.
No hosted remote has been created.

Preview verification: all three framebuffers are 240×135, scene and playback
controls work, native scale is exact, and mobile layout has no horizontal
overflow. `preview/capture.cjs` reproduces the PNG exports and comparison image
using Playwright and Chrome. In this environment:

```sh
NODE_PATH=/opt/homebrew/lib/node_modules node preview/capture.cjs
```

The upstream baseline build attempt downloaded its declared dependencies but
exited unsuccessfully before a usable firmware was produced; the captured
output did not identify a compiler error. This is not evidence about the bug
being investigated in the other session. Further firmware work is deferred
until the visual direction is selected and the forthcoming patch is supplied.

## Findings — 2026-09-12

The workspace was empty. PlatformIO 6.1.19, ESP32 platforms/toolchains,
Arduino CLI 1.4.1, and esptool are installed. Arduino CLI currently lists only
the AVR core; PlatformIO is the practical ESP32 build route. A USB device with
Espressif VID:PID 303A:1001 is present at `/dev/cu.usbmodem2101`; its firmware
and precise board identity have not been interrogated or changed.

Hardware: 240×135 display, ESP32-S3, 8 MB flash, 2.4 GHz Wi-Fi and BLE.
Bluetooth Classic discovery is unavailable. Advertised BLE names are optional;
some names arrive in scan responses, which require active scanning. The visual
should work with unnamed devices and never assume a phone's name is visible.

## Existing foundations

| Project | Fit | Tradeoff |
| --- | --- | --- |
| [GhostBLE](https://github.com/SmonSE/GhostBLE) | Recommended BLE foundation: ADV support, name/RSSI extraction, NimBLE, PlatformIO, MIT license | Includes GATT analysis, logging and other features beyond an ambient visualization; its Wi-Fi dashboard is not a Wi-Fi network scanner |
| [Bruce](https://github.com/BruceDevices/firmware) | Broad Wi-Fi/BLE firmware with existing menus and scanner features | Larger build/dependency surface; AGPL project |
| [Wi-Fi & BLE Radar ADV](https://github.com/Zeloksa/Cardputer-ADV-WiFi-BLE-Radar) | Close visual reference with a retro CRT interface | Published as compiled binaries only, so not a source foundation for this modification |

GhostBLE is cloned, unmodified, at `research/GhostBLE`, revision
`0b4ad45c43517395098f103180773e06b3961af4`. Its actual PlatformIO environment is
`ghostble_cardputer` (the README's shorter build example is stale).

```sh
cd research/GhostBLE
pio run -e ghostble_cardputer
```

## Proposed mode: NEON // FIELD

Black background, cyan geometry, magenta discovery pulses, amber unnamed
signals. Names float around a stylized scanner field. RSSI drives brightness
and radial position; a hash of the observation identity supplies a stable
decorative angle. These positions are not measured bearings or distances.
New observations briefly resolve from scrambled glyphs into readable names;
stale observations fade away. Show only a few labels at once on this tiny
screen, with a selected device's full name in a ticker.

Alternative renderers can use the same observations: falling name fragments
or neon building signs. A desktop preview should use explicitly synthetic
observations, with the same 240×135 logical resolution.

## Integration outline from source inspection

1. Add a bounded observation store containing identity, advertised name,
   smoothed RSSI and last-seen timestamp. Give unnamed observations a short
   session alias; keep manufacturer guesses distinct from advertised names.
2. Publish observations in `src/infrastructure/ble/ble_scanner.cpp` after
   obtaining each scan result, before `parseDeviceInfo` and its early filters.
   Repeated advertisements must refresh RSSI and timestamps even if the
   analysis layer deduplicates them. The current scan window is four seconds.
3. Transfer copies through a queue or a short locked snapshot. Do not read
   the scanner's mutable vectors directly from the renderer.
4. Add the mode under `src/ui/` and select it from the main menu. Integrate
   rendering in `GhostBLE.ino` / the existing UI lifecycle, ensuring mascot
   animation tasks cannot draw over the mode.
5. Keep animation independent of scanning, targeting 20–30 FPS initially.
   A 16-bit full-screen buffer needs 64,800 bytes, so measure free heap before
   choosing full-screen buffering alongside BLE.
6. Provide an observation-only scan path for this mode. The existing scanner
   can connect to GATT devices; passive advertisement scanning alone does
   not guarantee the rest of its processing remains connection-free.
7. Add Wi-Fi observations later through a separate adapter. Schedule radio
   use deliberately and distinguish SSIDs from BLE names in the display.

Validation needed before hardware use: baseline firmware build, bounded-store
behavior (expiry, repeated observations and long/empty names), animation heap
and frame timing on the ADV, keyboard exit and mode switching. No flashing has
been performed.

Sources: [M5Stack ADV specifications](https://docs.m5stack.com/en/core/Cardputer-Adv),
[Espressif ESP32-S3 Bluetooth documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/bluetooth/index.html),
and the linked repositories plus the local GhostBLE source.
