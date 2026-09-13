# LABScon visualizations

Firmware boots into city, replacing the original mascot home screen. GhostBLE
utilities remain in the menu. All views share observations, one framebuffer and
the audio playback session.

| Key | Action |
| --- | --- |
| 1 / 2 / 3 | City / radar / signal rain |
| Comma / slash | Previous / next device page; pauses automatic paging |
| 0 | Resume automatic paging every six seconds |
| A | Toggle passive / active scanning to request advertised names |
| P | Pause scanning; animations continue and stale observations expire |
| T | Hide/show findings; scanning and audio continue |
| B / N | Toggle music / next SD track |
| F / M or X | Toggle suspicious-device alerts / mute music and alerts |
| - / = | Lower / raise volume |
| H / S | Full-screen scrollable help / rendering time and free heap |
| Semicolon / period | Scroll help up / down, no Fn |
| D | Sleep / wake display; audio continues |
| Esc/backtick, Q | Open tools/settings menu; close help first if visible |

## Crowds and names

T suppresses finding labels, counts and radar contacts without stopping the
scanner or audio. Signal rain uses decorative hex glyphs while findings are
hidden; these are background artwork, not demo devices. H and menu Show Help
share one scrollable page built from the actual binding table. Help opened from
the menu returns there and does not start music or a new scan window; help opened
over a visualization leaves its audio/scanning running. M/X remains available
to mute while reading help.

City shows six large signs in small scenes, twelve compact signs in crowds.
Radar lists eight labels and plots all retained observations; angles are
decorative, **not measured bearings**. Rain has twelve name labels over glyph
streams. LABScon uses custom pixel lettering, not official logo artwork.

The bounded table retains up to 96 live observations. Retention favors flagged
matches, then named devices, then anonymous devices. Anonymous traffic cannot
evict a live named observation. Pages favor named and flagged observations;
otherwise spare slots page through ordinary anonymous devices. Names and flagged
matches stay on each page when they fit. Identity ordering avoids RSSI-induced
label shuffling. Counts describe the retained live table, not every nearby
device. More than 96 named devices can still cause older observations to be
replaced.

Names scroll within labels. ANON-xxxx means a real observation without an
advertised name, not injected demo data. Advertised names survive nameless
packets, fade after 12 seconds in city and expire after 20 seconds. Rotating
addresses can appear as new observations. Names are bounded to 24 ASCII
characters; unsupported bytes display as question marks.

Active mode transmits BLE scan requests, but does not pair or connect. Devices
that expose names only over GATT, or no name at all, can remain anonymous.
Callback-only delivery prevents an initial anonymous result list from excluding
later named advertisements. A 150 ms scan-response timeout limits pending
responses; low-heap detection stops a scan window for retry.

## Flagged platforms

See [watchlist research and limitations](device-watchlist.md). Matches are
heuristic identifiers, never proof of malicious intent. Amber labels, an
exclamation marker and slowly pulsing borders/markers identify flagged matches.
Ordinary new discoveries do not blink, and weak RSSI no longer uses amber.
Inferred platform labels start with a question mark and are not counted as
advertised names. Mesh/vehicle/wearable recognition is informational.

## Sound

See [soundtrack setup and licensing](soundtrack.md) for SD tracks and conversion.
Music and suspicious-only alerts start on, respect the saved master Audio toggle,
and continue across number-key view changes without restarting. Regular devices
and newly resolved names are silent. New watchlist matches trigger a 1.846-second
SD alarm; crowds coalesce behind a five-second cooldown. Repeated advertisements
from a retained flagged observation do not retrigger it. Turning F on also alerts
once if a flagged device is already visible. Leaving the visualization stops audio.

See the [complete Cardputer keymap](keymap.md), including legacy utility controls.

## Budgets and validation

Core renderers and selection have no Arduino dependency. One shared 4-bit
240×135 canvas occupies 16,200 bytes. Switching views allocates no second canvas.
The live table is copied to a snapshot under a short critical section. Frames
target 20 FPS, dropping to a 10 FPS budget if drawing/pushing costs over 40 ms.
These are limits, not measured hardware performance claims.

Build with `pio run -d firmware -e ghostble_cardputer`. Host tests:

```sh
cmake -S tests -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

Sanitized tests cover storage, names, priority/paging, expiry/rollover, effects
and pixel bounds in all views. Three optional output arguments to
`build/host/visualization_test` write city/radar/rain PPMs with synthetic fixtures.

Hardware validation remains necessary: dense active scans, view switching,
minimum heap, SD latency/removal, simultaneous music/effects, display sleep,
menu cleanup and long-running stability. **Do not flash without fresh user
approval. No backup is requested.** Wi-Fi visualization is not implemented.
