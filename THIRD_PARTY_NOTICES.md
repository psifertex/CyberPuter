# Third-party notices and credits

The root [MIT license](LICENSE) covers Cyberputer's original code and
documentation. It does not replace third-party licenses or grant trademark rights.

## GhostBLE

The firmware derives from [GhostBLE](https://github.com/SmonSE/GhostBLE) by
SmonSE, imported at revision `0b4ad45c43517395098f103180773e06b3961af4`.
Copyright (c) 2026 SmonSE. The full [upstream MIT notice](firmware/LICENSE)
is retained and applies to the imported code and assets. GhostBLE supplies the
hardware integration, BLE tools, menus, logging, GPS support and utility assets.

## SD recordings

The recordings are **CC0 1.0**, not MIT. Their source links, artist credits,
license links and conversion details are retained with the files:

- [Music credits](sdcard/cyberputer/music/CREDITS.md): Ruskerdax and Joth.
- [Alert credits](sdcard/cyberputer/sfx/CREDITS.md): EZduzziteh.

No artist endorsement is implied.

## Build dependencies

PlatformIO downloads these libraries separately. Preserve the license and
copyright notices supplied with the exact versions used in a distribution.

| Component | Credit / source | License |
| --- | --- | --- |
| M5Cardputer | [M5Stack](https://github.com/m5stack/M5Cardputer) | MIT; bundled Adafruit TCA8418 driver is BSD |
| IRremote (M5Cardputer dependency) | [Arduino-IRremote contributors](https://github.com/Arduino-IRremote/Arduino-IRremote) | MIT |
| M5Unified | [M5Stack and lovyan03](https://github.com/m5stack/M5Unified) | MIT |
| M5GFX | [M5Stack and lovyan03](https://github.com/m5stack/M5GFX) | MIT; bundled components retain their notices |
| NimBLE-Arduino | [h2zero and contributors](https://github.com/h2zero/NimBLE-Arduino) | Apache-2.0; bundled NimBLE/tinycrypt retain their notices |
| AsyncTCP | [Hristo Gochkov, Mathieu Carbou and contributors](https://github.com/ESP32Async/AsyncTCP) | LGPL-3.0 |
| ESPAsyncWebServer | [Hristo Gochkov, Mathieu Carbou, Emil Muratov and contributors](https://github.com/ESP32Async/ESPAsyncWebServer) | LGPL-3.0 |
| TinyGPSPlus | [Mikal Hart and contributors](https://github.com/mikalhart/TinyGPSPlus) | LGPL-2.1-or-later |
| Arduino-ESP32 | [Espressif and contributors](https://github.com/espressif/arduino-esp32) | LGPL-2.1; bundled components retain their licenses |
| ESP-IDF | [Espressif and contributors](https://github.com/espressif/esp-idf) | Apache-2.0 plus component-specific licenses |
| PlatformIO / pioarduino | [PlatformIO](https://github.com/platformio/platformio-core), [pioarduino](https://github.com/pioarduino/platform-espressif32) | Build tooling; see each project's license |

A compiled firmware image is not wholly MIT. Binary redistribution must also
satisfy dependency licenses, including applicable LGPL source, notice and
relinking requirements. See [release guidance](CONTRIBUTING.md#releases).
Links here are credits, not a substitute for required distribution materials.

## Visual references

Neon Odyssey is original procedural code inspired by classic demoscene effects:

- [Antonin Carette's tunnel explanation](https://carette.xyz/posts/the_tunnel_effect_demoscene/).
- [Ponceto's DOSFX effect collection](https://github.com/ponceto/dosfx).

No DOSFX code or artwork is included. Protocol signature sources are documented
with the [watchlist rules](docs/device-watchlist.md).

LABScon is used as conference-themed text in custom pixel lettering.
Product names and marks belong to their respective owners; no affiliation or
endorsement is implied.
