# Find My network observations and useful scanner metadata

Apple Find My advertisements can provide more useful context than a generic
alert: a broad advertised device class, a coarse advertised battery state,
the kind of finding frame, and local observation history. These are suitable
for informational details. They do not establish that a device is following
someone, identify its owner, or reliably identify an exact product.

The recommended immediate policy is to make Apple Find My watchlist emphasis
and sound opt-in, initially off, while retaining its informational protocol
label. The best subsequent improvement is a compact passive details view.
Selected-device Bluetooth metadata retrieval is a separate, feasible extension
that can sometimes resolve an actual model name.

This assessment covers public primary sources available on **2026-09-13** and
the repository's ESP32-S3 Cardputer firmware. Firmware variants and Apple
products have changed since the original 2021 reverse engineering. In
particular, Apple released its second-generation AirTag on January 26, 2026;
old AirTag observations should not automatically be applied to that model.[^1]
No physical-device captures or connection experiments were performed for this
report. Recommendations below distinguish source-verified decoding from
behavior that still needs hardware validation.

## Existing firmware

The implementation paths have different responsibilities:

| Path | Relevant finding |
| --- | --- |
| `firmware/src/core/detection/device_classifier.*` | Recognizes exactly 29 manufacturer-data bytes beginning `4c 00 12 19`. Correctly labels the result `FIND MY FORMAT`; this is protocol evidence, not AirTag authentication. |
| `firmware/src/ui/visualization/visualization_view.cpp` | Feeds manufacturer data into the shared classifier. Active scanning requests scan responses; it does not perform GATT discovery. |
| `firmware/src/core/visualization/observations.*` | Already retains first-seen, last-seen, and smoothed RSSI. It does not retain the decoded Apple payload fields. |
| `firmware/src/core/findmy/findmy_payload_parser.*` | Separate legacy summary decoder. Its battery source byte and public-key-bit extraction are inconsistent with the primary implementations below. |
| `firmware/src/core/parsing/findmy_payload_parser.h` | An older, fully commented-out duplicate of that decoder; it is not an additional working parser. |
| `firmware/src/core/findmy/findmy_sound.cpp` | Already discovers AirTag, FMNA, and DULT services for a user-triggered sound action. This is an active connection path, separate from passive classification. |
| `firmware/src/core/parsing/fmdn_parser.*` | An older Google detector. Its comments equate `0x40` with owner nearby and `0x41` with owner separation; the current Google specification does not support that equivalence. |

The older `infrastructure/ble/ble_scanner.cpp` also has its own permissive Apple
detector and legacy Google alert text, including `Stalker?!`. Those paths should
not be used as evidence that a stricter visualization classification is a
threat assessment. Findings about these older paths are follow-up observations,
not a recommendation to expand the immediate toggle change into a scanner
rewrite.

## Passive Apple payload fields

Use one unambiguous indexing convention: `m` is the complete manufacturer data
**including** the little-endian Apple company ID; `p = m + 4` is the 25-byte
payload after `4c 00 12 19`. Validate the complete frame before reading fields.
OpenHaystack's original ESP32 implementation supplies the following layout.[^2]

| Manufacturer offset | Payload offset | Interpretation |
| --- | --- | --- |
| `m[0..1]` | — | Company identifier `4c 00` |
| `m[2]` | — | Apple type `0x12` |
| `m[3]` | — | Payload length `0x19` |
| `m[4]` | `p[0]` | Status |
| `m[5..26]` | `p[1..22]` | Public-key bytes 6 through 27 |
| `m[27]` | `p[23]` | Low two bits hold the original public-key byte 0's top two bits |
| `m[28]` | `p[24]` | Hint byte; retain raw |

The remaining key bytes come from the advertising address. In the address order
used by OpenHaystack, reconstruction replaces address byte 0's top two bits with
`(p[23] & 3) << 6`, then appends `p[1..22]`. Check NimBLE address byte order with a
known fixture before using this. A short hash of this reconstructed public key
could label the **current advertising identity**; it is not a permanent device
identifier.[^2]

### Advertised class

Current AirGuard's actual device-selection code uses
`(status & 0x30) >> 4`, with the following class values.[^3]

| Value | Historical class | Suitable wording |
| --- | --- | --- |
| `0` | Other Apple device | `APPLE DEVICE?` |
| `1` | AirTag / Durian | `AIRTAG CLASS` |
| `2` | Find My accessory / Hawkeye | `FIND MY ACCESSORY` |
| `3` | Headphones / HELE | `HEADPHONE CLASS` |

These are broad, self-advertised hints. Current AirGuard explicitly allows
upgrading a generic Find My accessory to an AirTag after retrieving a name,
with a code comment identifying second-generation AirTag as the reason.[^3]
Consequently, value 2 should not be labeled “third-party only,” and a class
other than 1 should not exclude AirTag hardware.

Some older AirGuard scan-filter masks still use `0x18`, while its current
device-selection function uses `0x30`. The filter alone is not the classifier.
Use the explicit selection logic above, retain the raw status, and validate
representative captures instead of copying a historical filter as a decoder.
The older AirGuard paper describes the same four class meanings.[^4]

### Battery and hint byte

Current AirGuard obtains the coarse battery value from
`(status >> 6) & 3`: 0 full, 1 medium, 2 low, 3 very low.[^5] This supports a
label such as `BAT: LOW (advertised)`, not a percentage or runtime estimate.
Emulators and unsupported implementations can use arbitrary status values.
An all-zero status must not be treated as independent proof of a healthy
physical battery.

The firmware currently reads `(p[24] >> 6) & 3` for battery and
`(p[23] >> 6) & 3` for key bits. The evidence instead supports `p[0]` for battery
and `p[23] & 3` for key bits. Correct these before reusing the summary decoder
in a details view. Preserve `p[24]` as raw data; do not relabel it as battery,
elapsed separation time, or a decoded security flag.

### Frame/state information

The complete `12 19` frame is associated with offline finding and, for
traditional Find My accessories, the separated state. The earlier protocol
research distinguishes a shorter nearby state that omits part of the public
key from the full separated frame.[^4] The present strict classifier recognizes
the latter; it is not an inventory of all nearby Apple devices.

Use `OFFLINE-FINDING FRAME` or `SEPARATED-FORMAT` as the observation. Avoid
“owner absent,” “reported lost,” “tracking you,” or a claimed number of minutes
since separation. Device-reported state can be inaccurate or simulated, and
physical proximity is not equivalent to a working owner-device Bluetooth
connection. The August 2026 DULT threat model specifically discusses owner
Bluetooth being disabled as a cause of incorrect separation inference.[^6]

## Information available at each access level

| Access | Useful output | Practical limit |
| --- | --- | --- |
| Passive advertisement reception | Format, broad advertised class, coarse battery, raw status, RSSI, observation times | No authenticated identity or movement verdict |
| BLE active scanning | Any additional name or advertised services returned in a scan response | Only what the advertiser chooses to return; no automatic access to GATT characteristics |
| Selected-device, unauthenticated GATT reads | Some devices expose a useful model/device name | Requires a connection; availability depends on product, state, and firmware |
| DULT metadata queries | Product/model/manufacturer, category, firmware and optional battery | A connection plus metadata request/response protocol; support must be discovered |
| Owner account/keys or physical identification | Owner-authorized location reports, or found-item identification | These are different workflows from the Cardputer's background BLE scanner |

The expected benefit of more aggressive scan-response collection is modest.
There is no verified universal Find My scan-response field that exposes an
owner name or exact product model. Treat extra local-name strings as advertised
text and keep that evidence distinct from inferred class labels.

Current AirGuard reads a string from service
`87290102-3C51-43B1-A1A9-11B9DC38478B`, characteristic
`6AA50003-6352-4D57-A7B4-003A416FBB0B`. Its decoder recognizes `left` and `right`
for AirPods and otherwise preserves the returned name.[^5] Its subtype
integration uses suitable names to refine generic Find My results into AirTag
or AirPods labels.[^7] This is strong implementation evidence for a future
explicit **Read details** action, but is not a promise every matching beacon
will accept the connection or expose this characteristic.

The public DULT accessory-protocol draft defines non-owner, unencrypted
connections and an information exchange for product data, manufacturer, model,
category, capabilities, firmware, and optional battery. It uses service
`15190001-12F4-C226-88ED-2AC5579F2A85` and characteristic
`8E0C0001-1D68-FB92-BF61-48377421680E`; metadata is requested with writes and
returned through indications. A metadata-only client would need an allowlist
of these queries, timeouts, string bounds, and response validation, rather than
calling the existing sound routine.[^8]

That accessory specification is the November 2024 `-00` Internet-Draft, shown
as expired in the checked Datatracker record. It is useful implementation
guidance, not a finalized RFC or evidence of universal deployment.[^8] The
threat-model draft has progressed to `-05`, dated August 6, 2026; it continues
to describe inconsistent accessory information and alert-fatigue problems.[^6]

## Recent security research and its relevance

The research reviewed does not establish a general method for a passive
scanner to recover the owner or decrypt another device's location history.
The recurring practical finding is that protocol-shaped broadcasts do not
authenticate hardware or trustworthy location claims.

| Work | Verified scope | Consequence for this firmware |
| --- | --- | --- |
| OpenHaystack and *Who Can Find My Devices?*, 2021 | Reverse engineered offline finding and built compatible beacons. The original location-access vulnerabilities required a different access context, including local applications; the project reports Apple fixed its most severe issue. [^9][^10] | A format match includes emulators and ordinary devices. Historical vulnerabilities are not an available passive identity decoder. |
| *Send My*, May 2021 | Demonstrated attacker-encoded data transmission through Find My, including an ESP32 sender. [^11] | Arbitrary transmitters can construct plausible finding advertisements; this does not let a bystander decode arbitrary genuine tags. |
| *Tracking You from a Thousand Miles Away!*, USENIX Security 2025 — nRootTag | Makes a Bluetooth-capable computer emit attacker-usable Find My beacons without root privileges, under the attack's execution and key-search assumptions. [^12] | Device class cannot prove physical AirTag hardware. The attack creates suitable keys; it does not recover a nearby genuine AirTag's existing private key. |
| *Symbolic verification of Apple's Find My location-tracking protocol*, October 2025 | Formal model of secrecy properties, with 10 of 12 lemmas verified and two timing out; authors identify abstraction and unmodeled privacy-property limits. [^13] | Useful cryptographic analysis, not an identity-extraction capability or a complete proof of every deployed implementation. |
| *A Relay a Day Keeps the AirTag Away*, April 2026 preprint | Demonstrates altered owner-visible locations through replay/relay of captured broadcasts. Experiments used researchers' own devices. [^14] | An observed key may be replayed elsewhere. The paper supplies no passive owner lookup and should not be read as proof of every current AirTag generation's behavior. |
| DULT threat model `-05`, August 2026 | Documents nonconformant tags, replay, implementation differences, false positives, and customizable detection needs. [^6] | Support selective alerts and explain the observation. A single matching advertisement is weak grounds for an alarm. |

The April 2026 paper reports approximately daily AirTag key rotation in its
experiments and distinguishes local beacon handling from cloud reports.[^14]
Apple's earlier security guide describes approximately 15-minute key changes
for offline Apple devices.[^15] These describe different device/state
contexts. Do not implement one universal Find My rotation timer or merge two
different addresses merely because they appeared close together in time.

## Identity, location, and observation limits

The advertised public key is usable for public-key operations, not as a
decryption secret. Apple's design derives changing keys to prevent easy
linkage, encrypts reports to the device's owner, and keeps finder and owner
identities separate.[^15] A locally reconstructed public key can support
short-lived deduplication, but it does not reveal an Apple Account, person,
phone number, item nickname, or location history.

Apple provides a different found-item route: an NFC tap on a found AirTag can
open a page containing its serial number, part of the registrant's phone
number, and a contact message when supplied. That requires the found-item
interaction and is not data exposed in the ordinary finding advertisement.[^16]

Longer local observation is useful context, but one stationary scanner cannot
tell whether a tag traveled with its user. The original AirGuard detector
combined repeated detections with elapsed time and geographic movement;
address changes could create a new entry.[^4] For this firmware, `seen 8m`,
`last 2s`, and an RSSI trend accurately describe local evidence. “Following”
would require additional movement evidence and a separately evaluated
detection algorithm. A crowded train, a fixed neighboring tag, scan gaps,
and address rotation remain realistic ambiguity sources.

## Google Find Hub requires separate treatment

Google's current specification explicitly puts Find Hub advertisements under
service-data UUID **FEAA**, with frame `0x40` or `0x41`, a 20- or 32-byte EID,
and optional hashed flags. This supports the stricter classifier's payload
length checks, rather than the older parser's single-byte acceptance or its
claim that FEAA is an undocumented substitute for FE2C.[^17]

`0x41` signals **unwanted-tracking protection mode**, which slows advertising
address rotation; `0x40` indicates that mode is not enabled. Neither by itself
establishes owner proximity or stalking. The optional flags, including battery,
are XOR-masked using material derived from the private ephemeral scalar; they
are not a plaintext battery byte a passive observer can generally decode from
the EID. Normal EID/address rotation averages 1024 seconds; protection mode
allows a much longer-lived address while the EID still rotates.[^17]

Recommendation: keep Apple's toggle explicitly Apple-scoped. If the product
later wants quieter treatment for all crowd-finding formats, make Google a
separate choice or name the combined setting clearly. Correct the old Google
state wording independently. Do not transplant Apple's battery/class decoder
into Google service data.

## Minimal follow-up

First, retain the off-by-default Apple highlight/sound setting and its neutral
format label. Decode a few bytes into bounded metadata attached to each
observation: raw status, broad class, coarse battery, and full-frame kind.
Show these alongside existing observation times and signal strength in a
detail panel. Keep inference provenance visible with words such as “class”
and “advertised.”

Repair the legacy decoder's two field mistakes before sharing it with the
visualizations, and avoid allocating long public-key strings in scan callbacks.
Raw hint/key fields belong in an optional technical view; their hex values add
little to a busy rain page. Treat observation expiry, MAC rotation, and
reappearance as explicit discontinuities instead of claiming continuous
presence.

Then evaluate a selected-device **Read details** action using discovered
metadata services. It should fail back to the passive label, cache results
only for the observed identity, and avoid automatic connection attempts across
the crowded device list. Readable text is still device-provided and should be
sanitized. Existing sound support is a useful connection reference, but the
metadata flow should not trigger sound as part of discovery.

Before shipping enrichment, validate with owned examples of first- and
second-generation AirTags, a third-party Find My accessory, AirPods, and an
offline Apple computer. Record actual firmware versions and compare full and
short frames, class values, available names, and connection behavior. Decoder
fixtures should vary status independently of hint byte, exercise all four
key-bit values, reject truncation, and verify address byte order. Until those
captures exist, precise model/state coverage remains unverified.

## Sources

All rolling webpages and repository files below were checked on 2026-09-13.
Repository `main` links identify the consulted implementation; future changes
may differ. Dates below refer to publication or the pinned paper/draft version,
not a search engine's relative-age label.

[^1]: Apple. [Apple introduces new AirTag with expanded connectivity range and improved findability](https://www.apple.com/newsroom/2026/01/apple-introduces-new-airtag-with-expanded-range-and-improved-findability/). January 26, 2026.
[^2]: Secure Mobile Networking Lab, TU Darmstadt. [OpenHaystack ESP32 firmware, `openhaystack_main.c`](https://github.com/seemoo-lab/openhaystack/blob/main/Firmware/ESP32/main/openhaystack_main.c), especially advertisement construction and `set_payload_from_key`.
[^3]: Secure Mobile Networking Lab, TU Darmstadt. [AirGuard `DeviceManager.kt`](https://github.com/seemoo-lab/AirGuard/blob/main/app/src/main/java/de/seemoo/at_tracking_detection/database/models/device/DeviceManager.kt), `calculateDeviceType` and second-generation AirTag override comment; class constants in [AirTag](https://github.com/seemoo-lab/AirGuard/blob/main/app/src/main/java/de/seemoo/at_tracking_detection/database/models/device/types/AirTag.kt), [AppleFindMy](https://github.com/seemoo-lab/AirGuard/blob/main/app/src/main/java/de/seemoo/at_tracking_detection/database/models/device/types/AppleFindMy.kt), and [AirPods](https://github.com/seemoo-lab/AirGuard/blob/main/app/src/main/java/de/seemoo/at_tracking_detection/database/models/device/types/AirPods.kt).
[^4]: Alexander Heinrich, Niklas Bittner, Matthias Hollick. [AirGuard — Protecting Android Users From Stalking Attacks By Apple Find My Devices](https://arxiv.org/pdf/2202.11813). February 23, 2022; sections 2.2, 3.5 and 4.1. Historical protocol/device observations, not current OS alert thresholds.
[^5]: Secure Mobile Networking Lab, TU Darmstadt. [AirGuard `AppleFindMy.kt`](https://github.com/seemoo-lab/AirGuard/blob/main/app/src/main/java/de/seemoo/at_tracking_detection/database/models/device/types/AppleFindMy.kt), `getBatteryState` and `getSubTypeName`.
[^6]: Maggie Delano, Jessie Lowell, Shailesh Prabhu. [DULT Threat Model, draft-ietf-dult-threat-model-05](https://datatracker.ietf.org/doc/html/draft-ietf-dult-threat-model-05). August 6, 2026; sections 4.1 and 4.3. Work in progress.
[^7]: Secure Mobile Networking Lab, TU Darmstadt. [AirGuard `DeviceSubTypeDetector.kt`](https://github.com/seemoo-lab/AirGuard/blob/main/app/src/main/java/de/seemoo/at_tracking_detection/util/ble/DeviceSubTypeDetector.kt), Apple metadata-name refinement.
[^8]: Brent Ledvina, David Lazarov, Ben Detwiler, Siddika Parlak Polatkan. [Detecting Unwanted Location Trackers Accessory Protocol, draft-ietf-dult-accessory-protocol-00](https://datatracker.ietf.org/doc/html/draft-ietf-dult-accessory-protocol-00). November 2024; sections 3.2.2, 3.11 and 3.12. Expired Internet-Draft, not an RFC.
[^9]: Secure Mobile Networking Lab, TU Darmstadt. [OpenHaystack project](https://github.com/seemoo-lab/openhaystack), research history and disclosed vulnerability status.
[^10]: Alexander Heinrich, Milan Stute, Tim Kornhuber, Matthias Hollick. [Who Can Find My Devices? Security and Privacy of Apple's Crowd-Sourced Bluetooth Location Tracking System](https://arxiv.org/abs/2103.02282). Proceedings on Privacy Enhancing Technologies, 2021.
[^11]: Fabian Bräunlein, Positive Security. [Send My: Arbitrary data transmission via Apple's Find My network](https://positive.security/blog/send-my). May 12, 2021.
[^12]: Junming Chen, Xiaoyue Ma, Lannan Luo, Qiang Zeng. [Tracking You from a Thousand Miles Away! Turning a Bluetooth Device into an Apple AirTag Without Root Privileges](https://www.usenix.org/conference/usenixsecurity25/presentation/chen-junming). USENIX Security, August 2025; conference page includes the paper.
[^13]: Vaishnavi Sundararajan, Rithwik. [Symbolic verification of Apple's Find My location-tracking protocol](https://arxiv.org/html/2510.14589v2). October 21, 2025 revision; sections 4.4–5. Preprint.
[^14]: Gabriel K. Gegenhuber, Leonid Liadveikin, Florian Holzbauer, Sebastian Strobl. [A Relay a Day Keeps the AirTag Away: Practical Relay Attacks on Apple's AirTags](https://arxiv.org/html/2604.10138v1). April 11, 2026. Preprint.
[^15]: Apple. [Find My security](https://support.apple.com/en-euro/guide/security/sec6cbc80fd0/web). February 18, 2021 publication date on the checked page. Offline Apple-device key rotation and encrypted reporting design.
[^16]: Apple. [What to do if you get an alert that an AirTag, set of AirPods, Find My network accessory, or compatible Bluetooth location-tracking device is with you](https://support.apple.com/en-us/119874). March 19, 2026.
[^17]: Google. [Find Hub Network Accessory Specification](https://developers.google.com/nearby/fast-pair/specifications/extensions/fmdn), advertised frames, unwanted-tracking protection mode, hashed flags, and ID rotation. Rolling specification.
