# Visualizations

Cyberputer boots into Neon city. Scene changes share the same observation
table, framebuffer and audio session. See the [keymap](keymap.md) for controls.

## Scenes

**Neon city** places device names on buildings with LABScon signage. Small
scenes show six large signs; crowds show twelve compact labels.

**Neon radar** lists eight labels per page and plots all retained observations.
Angles are decorative, not measured bearings.

**Signal rain** displays twelve name labels over falling glyph streams.
When findings are hidden, decorative hexadecimal glyphs fill the background.

**Neon Odyssey** is a device-free, 54-second eye-candy loop: 18 seconds each
of twisting octagonal tunnel, rotating circuit grid and radial warp field.
Moving plasma underlays the effects. Names, counts and pagination are absent;
status messages and statistics can overlay the bottom. Animation is time-driven,
not synchronized to music. See [visual references](../THIRD_PARTY_NOTICES.md#visual-references).

## Observations and paging

The fixed table retains up to 96 live observations. Retention favors watchlist
matches, then named devices, then anonymous devices. Anonymous traffic cannot
evict a live named observation; a full table can replace older entries in the
same priority class.

Device pages are distinct slices of one ordered list, with watchlist matches
first, then names, then anonymous entries. The final page can be partial.
Identity ordering prevents RSSI-driven shuffling. Arrivals, expiry and newly
learned names or classifications can still move entries. Counts describe the
retained table, not every nearby device.

Names scroll within labels and are limited to 24 ASCII characters; unsupported
bytes display as question marks. ANON labels mean real observations without an
advertised name. Inferred platform labels start with a question mark and do not
count as advertised names. See the [watchlist](device-watchlist.md) for interpretation.

Advertised names survive nameless packets. Observations fade after 12 seconds
in city and expire after 20 seconds. Rotating addresses can appear as new entries.

## Scanning

Passive scanning listens for advertisements. Active scanning additionally
requests scan responses, which may contain names. Neither visualization scan
mode pairs or connects over GATT. Devices that do not advertise a name can
remain anonymous. The menu's inherited connection tools are separate.

Pausing scanning leaves animation running while stale entries expire.
Hiding findings suppresses labels, counts and radar contacts without stopping
scanning or audio. Display sleep also leaves scanning and audio running.

Callback-only delivery avoids a retained radio result list. A 150 ms
scan-response timeout bounds pending responses; low-heap detection stops a
scan window for retry. Bluetooth Classic and Wi-Fi visualizations are unsupported.

## Rendering budget

A single 4-bit 240×135 framebuffer occupies 16,200 bytes. Scene changes allocate
no second canvas. The table is copied to a snapshot under a short critical section.
Frames target 20 FPS, falling back to a 10 FPS budget if drawing and display
transfer exceed 40 ms. These are scheduling targets, not guaranteed frame rates.

Odyssey uses 80×45 plasma/circuit samples with native-resolution lines and no
additional heap allocation. SD stalls and heavy radio activity can affect
performance; the statistics overlay reports frame cost and free heap.
