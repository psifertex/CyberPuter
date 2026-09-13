# Contributing

## Architecture

- `firmware/src/core/visualization/`: Arduino-independent observation, rendering and audio logic.
- `firmware/src/core/detection/`: bounded platform signature classifier.
- `firmware/src/ui/visualization/`: device display, scanner and SD audio integration.
- `tests/`: native regression tests.
- `sdcard/`: redistributable audio with accompanying credits.

Keep scan callbacks bounded and avoid filesystem access in the UI update path.
Preserve framebuffer and audio-buffer ownership across asynchronous task stops.
See [coding conventions](firmware/CODING_GUIDELINES.md).

## Tests

Requires CMake 3.16+ and a C++17 Clang or GCC toolchain. From the root:

```sh
cmake -S tests -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

The three test targets cover observation storage, paging, classifiers, renderer
bounds, clock rollover, alert scheduling and WAV parsing. Address and undefined
behavior sanitizers are enabled on Clang/GCC. Tests use synthetic identities and
the bundled recordings, never live scans.

The visualization test accepts five optional positional output paths for
city, radar, rain, help and Odyssey PPM captures, respectively.

Build the supported firmware using the [build guide](docs/building.md).
Hardware validation should cover dense active scans, scene switching, minimum
heap, SD stalls/removal, music plus alerts, display sleep and long-running use.
Host tests cannot establish radio behavior or device frame rate.

## Documentation and rules

Document current behavior, not session history. Keyboard mappings belong only
in [docs/keymap.md](docs/keymap.md); other guides link to it. On-device help and
dispatch share `firmware/src/core/visualization/controls.h`.
Keep those two representations in sync.

A classifier rule needs a primary source, positive and negative fixtures, and
a precise label. Do not equate a spoofable signature with malicious intent or
exact hardware identity. Keep credentials, scans, GPS logs, dumps and local
build artifacts out of commits. Use synthetic fixtures for reports and tests.

## Releases

The version source is `firmware/src/config/version.h`. Increment minor for
feature releases and patch for fixes; documentation-only edits do not require
a firmware version change. Do not reuse a distributed version.
Use the stable filename `cyberputer.bin`; the device displays the version.

Before distributing binaries, include the matching source revision, build
configuration, dependency versions and required license/copyright notices.
For statically linked LGPL dependencies, provide the corresponding library
source and the applicable application object/relink materials and instructions,
not just an application image or a link to upstream. Review the license terms
for the exact dependencies and distribution method.

The CI workflow builds and tests source; it does not publish firmware releases.
See [third-party notices](THIRD_PARTY_NOTICES.md) for dependency provenance.
