# SD soundtrack playlist

Copy the repository's `sdcard/cyberputer` folder to the **root of the microSD
card**. It should contain:

```text
/cyberputer/music/01-Cybershaman.wav
/cyberputer/music/02-Cyberpunk-Moonlight-Sonata.wav
/cyberputer/music/CREDITS.md
/cyberputer/sfx/suspicious.wav
/cyberputer/sfx/CREDITS.md
```

Insert the card before boot. **B** toggles music, **N** selects the next track,
**F** toggles suspicious-device alerts, **M / X** silences both and **−/=**
change volume. Music and alerts start on when entering the visualizations,
subject to GhostBLE's saved master Audio setting.
Selection works while muted; B restarts the selected song. At the end of a song
the playlist advances automatically and wraps. Switching visualization modes
keeps the music playing. The music status/name API lets the UI show the selected
filename and missing-card, missing-file, unsupported-WAV or read-error messages.

The two bundled files contain complete recordings, converted to mono 16-bit PCM
WAV at 22,050 Hz and gain 0.72. No music samples are embedded in the firmware;
The alert is a separate SD sample, not an embedded tone; missing assets are
reported in the footer.

## Add recordings

The suspicious-only alarm is [Alarm by EZduzziteh](https://opengameart.org/content/alarm-1),
CC0. Credits, hashes and conversion command are in
`sdcard/cyberputer/sfx/CREDITS.md`. Keep its directory separate from the playlist.
F enables/disables alerts; ordinary discoveries and name resolution are silent.
Newly flagged devices alert once, with a five-second global crowd cooldown;
F on also alerts for a flagged device already visible. Classification is heuristic,
not proof of malicious activity. Music and alerts begin enabled on entry, but
the saved master Audio mute still takes precedence.

The bundled alert is normalized to approximately -3 dBFS peak. During playback,
music channel gain drops by about 21 dB, then returns to its prior setting when
the alert channel becomes idle (including after an asynchronous F-off stop).
The music keeps advancing; it is not paused/restarted. Alerts use the same nominal
channel level as music, while master volume and M/X mute remain in control.
Muting or leaving a view relinquishes the temporary gain state, and failed
alert submissions immediately restore music. No additional PCM buffer is needed.

**After updating the alert file, replace `/cyberputer/sfx/suspicious.wav` on the
card and reboot** to reload the cache. Start testing at a low master volume;
the new sample is substantially louder than the old version.

The worker loads the alert once into an immutable 32 KB cache. Replacement alerts
must be mono signed 16-bit PCM WAV at 8 kHz and at most two seconds (32,000 PCM
bytes). Reload by rebooting. Missing/invalid files produce a footer message;
F off/on retries a failed load, with no generic chirp fallback. This cache remains
valid across menu exits so asynchronous speaker stops cannot access freed memory.

## Add music recordings

The player discovers up to 16 `.wav` files in `/cyberputer/music`, sorts their
filenames alphabetically, and displays filenames as track names. Use numeric
prefixes to choose order. Filenames must fit within 47 bytes; dotfiles and
subdirectories are ignored. Directory discovery examines at most 256 entries.
Keep this directory dedicated to music. The playlist is cached for the current
boot; reboot after adding/removing tracks. Insert/remove the card with music and
logging stopped; a missing card needs to be inserted before reboot because the
player deliberately does not remount or reconfigure the shared SD hardware.

Convert any recording you have permission to use with ffmpeg installed:

```sh
python3 scripts/prepare_soundtrack.py input.flac sdcard/cyberputer/music/03-My-Track.wav
```

Copy the resulting WAV to the same directory on the card. The script refuses to
overwrite existing output. Runtime accepts mono signed 16-bit PCM RIFF WAV at
8–44.1 kHz; MP3, FLAC, stereo, float and compressed WAV must first be converted.
The script's 22.05 kHz output is recommended for the speaker and available RAM.

## Recordings and rights

**“Cybershaman” by Ruskerdax — CC0 1.0.** Complete 3:18.5 instrumental recorded
with Roland Juno-6/Jupiter-8/TB-303, guitar and other instruments.

- Artist's upload/license: https://opengameart.org/content/cybershaman
- Original: https://opengameart.org/sites/default/files/ruskerdax_-_cybershaman.flac
- Artist: https://soundcloud.com/ruskerdax
- Original SHA-256: `f52d71e66bf900c14faf7bc106668d1aebed83a924c85c19727d967c51e909fc`

**“Cyberpunk Moonlight Sonata” (v2) by Joth — CC0 1.0.** Electronic cyberpunk
reinterpretation of Beethoven's composition, complete recording.

- Artist's upload/license: https://opengameart.org/content/cyberpunk-moonlight-sonata
- Original: https://opengameart.org/sites/default/files/Cyberpunk%20Moonlight%20Sonata%20v2.mp3
- Original SHA-256: `c195845dab8b18c1086dafd1d70e5efd5350db3762d58a4abdaaee36bc82bff8`

Both artist upload pages explicitly mark their recordings CC0, verified
September 12, 2026. Attribution is appreciated but is not a condition of CC0.
No endorsement is implied. The derivative format conversions remain CC0,
independent of the firmware license:
https://creativecommons.org/publicdomain/zero/1.0/
(legal code: https://creativecommons.org/publicdomain/zero/1.0/legalcode).

## Bounded playback and SD coexistence

A low-priority worker reads SD audio into four fixed 2,048-sample buffers
(16,384 PCM bytes) with a 4 KB task stack and a 768-byte bounded filename table.
Only this worker owns the audio file. UI updates never read/open/seek SD files
or wait for SD. The worker tries the existing logger/WiGLE mutex without waiting;
Arduino filesystem/SPI locking covers the legacy clients that use other locks,
including screenshots and GATT logs. It never calls `SD.begin()` or `SD.end()`.
Filesystem internals and the one worker task require their normal small heap
allocations; music file size does not affect RAM usage.

The speaker retains at most two PCM pointers. Buffer ownership prevents worker
refills until those pointers are released, including asynchronous stop/skip.
The worker prefetches up to two additional blocks; at 22.05 kHz the four blocks
span about 372 ms. Long SD stalls or heavy writes can cause audible gaps, but
the visuals continue. Stalled/removed media produces a status message instead
of repeated retry loops; B or N retries. Hardware crowd/SD testing remains
necessary. Host tests cover WAV bounds/format rejection and both bundled files.
