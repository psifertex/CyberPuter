# LABScon city mode

Firmware implementation: `firmware/` is a tracked import of GhostBLE
`0b4ad45c43517395098f103180773e06b3961af4`. Its MIT license remains in
`firmware/LICENSE`. `research/GhostBLE` is the unchanged reference submodule.

## Use

From GhostBLE's home screen, press **V**, or choose **CYBERPUTER → LABScon Neon
City** in the main menu. The city borrows the existing scan task after any
ongoing analysis finishes/yields; a footer indicates the transition. A new
legacy GATT analysis is not started while the city is open.

| Key | Action |
| --- | --- |
| Esc/backtick, M or Q | Return to menu after scanner cleanup; restore the previous scan-enabled state |
| S | Pause/resume BLE scanning (animation continues; old signs still expire) |
| D | Sleep/wake the display |
| P | Toggle render-time and free-heap readout |

The renderer targets 240×135 pixels. LABScon uses custom pixel lettering.
Advertised device names appear on six signs; long names scroll. More than six
observations rotate through the signs every eight seconds. New discoveries
flash briefly, weak signals use amber, and observations fade after 12 seconds
and disappear after 20. Unnamed devices use `ANON-xxxx` session labels.

This mode uses passive BLE advertisement scans. It does not pair, make GATT
connections, or add city observations to SD/web logs. Names available only in
active scan responses are not discovered in this first version. A pre-existing
legacy analysis may finish its current device before handing over the radio.
The city's fresh table updates repeated observations rather than inheriting
the analysis layer's deduplication. Identity is address plus address type, not
the name; rotating BLE addresses can appear as new observations.

## Architecture and budgets

- `src/core/visualization/observations.*`: allocation-free, 24-entry observation
  table; sanitized 24-character names, RSSI smoothing, expiry, oldest eviction.
  A table occupies 1,152 bytes on the tested host ABI.
- `src/core/visualization/renderer.*`: hardware-independent `Surface`, `Frame`,
  and `Renderer` interface. `rendererFor()` is the single mode lookup. The city
  is procedural pixel art with a fixed 16-color palette and no bitmap assets.
- `src/ui/visualization/visualization_view.*`: M5Canvas adapter and asynchronous
  scan-task handoff. One 4-bit sprite occupies 16,200 bytes plus palette/library
  overhead. The live table and UI snapshot are separate, about 2.3 KB combined,
  copied under a short critical section. NimBLE retains at most 24 scan results.
- Legacy drawing helpers and city LCD pushes share a recursive display mutex;
  legacy drawing is suppressed while the city owns the screen. No mascot task
  is forcibly deleted. Screenshot servicing pauses until this mode closes.
- A frame starts at most every 50 ms (20 FPS target). A measured render+push
  cost above 40 ms switches to a 100 ms budget (10 FPS). This is a limit and
  fallback, not a claim of measured hardware frame rate.
- The sprite is allocated once on entry, freed on exit, and shared by future
  renderers. Entry requires a sufficiently large internal-memory block and
  48 KiB remaining heap headroom. Allocation failure leaves the previous view
  in place and reports the reason on Serial.

## Deferred renderers

- TODO **Radar**: implement `Mode::Radar` using the same observation snapshot,
  palette and surface. Decorative angles must not imply measured bearing.
- TODO **Signal rain**: implement `Mode::Rain` with a large LABScon wordmark and
  device-name glyph streams using the same buffer.
- TODO expose mode cycling only after measuring frame time, minimum free heap,
  and repeated transitions on the ADV. `setMode()` already supports selecting
  a registered renderer without reallocating the framebuffer. Missing modes
  return false and keep the current renderer; no placeholder mode can blank it.
- TODO evaluate optional active scanning for devices whose names are only in
  scan responses, and a Wi-Fi adapter if desired.

## Build and validation

```sh
pio run -d firmware -e ghostble_cardputer
cmake -S tests -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

Host tests run with AddressSanitizer/UndefinedBehaviorSanitizer and cover
repeated observations, distinct address types, missing/long/control-byte names,
bounded capacity and eviction, expiry across millisecond rollover, unavailable
modes, and framebuffer bounds across animation phases. Passing an output path
to `build/host/visualization_test` writes a 240×135 PPM from the actual C++ city
renderer using synthetic observations.

Before calling this hardware-validated, check entry during an active legacy
scan, rapid entry/exit, pause/resume, display sleep, dense BLE environments,
allocation failure, and sustained frame/heap readings on the Cardputer ADV.
No device has been flashed as part of this implementation.

The separately supplied GhostESP setup-wizard report does not apply to this
codebase and was excluded at the user's request.
