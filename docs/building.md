# Building and installation

## Requirements

- M5Stack Cardputer ADV (ESP32-S3, 240×135 display, 8 MB flash, no PSRAM).
- PlatformIO Core 6.1.19 and its supported Python runtime.
- Internet access for the first toolchain/library download.
- A data-capable USB cable for USB installation.

The platform version and direct library versions are specified in
[platformio.ini](../firmware/platformio.ini). No submodule checkout is required.
Other inherited board configurations are not validated Cyberputer targets.

## Build

Run from the repository root:

```sh
pio run -d firmware -e ghostble_cardputer
```

Outputs under `firmware/.pio/build/ghostble_cardputer/`:

| File | Purpose / flash offset |
| --- | --- |
| bootloader.bin | Bootloader, 0x0 |
| partitions.bin | Partition table, 0x8000 |
| firmware.bin | Application, 0x10000 |

The application must fit the 3 MB app slot in
[partitions.csv](../firmware/partitions.csv), regardless of PlatformIO's
larger reported flash capacity.

## SD firmware loaders

Copy the application image under the stable name `cyberputer.bin`:

```sh
mkdir -p build
cp firmware/.pio/build/ghostble_cardputer/firmware.bin build/cyberputer.bin
```

Use a loader that accepts ESP32-S3 application images and has a compatible
partition layout. This file is **not a merged image** and must not be written
at address zero. Firmware loaders differ; follow the loader's instructions.
The running release version is visible in the menu and on-device help.

## USB installation

Installing firmware overwrites the installed application; partition changes
can make existing data inaccessible. Confirm the target port and save any data
you want to keep before proceeding. Do not disconnect during writing.

PlatformIO can build and upload the matching bootloader, partitions and app:

```sh
pio device list
pio run -d firmware -e ghostble_cardputer -t upload --upload-port PORT
```

Replace `PORT` with the detected device port. The upload command is an explicit
device-changing operation, unlike a plain build.

For playback, follow [SD audio setup](soundtrack.md). For host-side verification,
see [contributing](../CONTRIBUTING.md).
