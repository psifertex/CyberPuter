# SD audio

Copy [sdcard/cyberputer](../sdcard/cyberputer) to the **root of the microSD card**:

```text
/cyberputer/music/01-Cybershaman.wav
/cyberputer/music/02-Cyberpunk-Moonlight-Sonata.wav
/cyberputer/music/CREDITS.md
/cyberputer/sfx/suspicious.wav
/cyberputer/sfx/CREDITS.md
```

Insert the card before boot. See the [keymap](keymap.md) for playback controls
and defaults. Missing/unsupported files and read errors appear in the footer.
There is no embedded music or fallback alert tone.

## Playlist

The player discovers up to 16 WAV files in `/cyberputer/music`, sorts filenames
alphabetically and uses them as track names. Numeric prefixes control order.
Filenames must fit within 47 bytes; dotfiles and subdirectories are ignored.
Discovery examines at most 256 entries. Keep this directory dedicated to music.

Tracks advance automatically and wrap. Scene changes do not restart playback.
Selection works while music is disabled; enabling music starts the selected track.
The playlist is cached until reboot, so reboot after changing the files.

### Add recordings

Convert a recording you have permission to use with Python 3 and ffmpeg:

```sh
python3 scripts/prepare_soundtrack.py input.flac sdcard/cyberputer/music/03-My-Track.wav
```

The script refuses to overwrite output and produces mono signed 16-bit PCM WAV
at 22,050 Hz with gain 0.72. Runtime accepts mono signed 16-bit PCM RIFF WAV at
8–44.1 kHz. MP3, FLAC, stereo, floating-point and compressed WAV require conversion.

## Watchlist alarm

Only newly flagged observations trigger the alarm. Ordinary discoveries and
name resolution are silent. A five-second global cooldown coalesces crowds;
repeated advertisements from a retained observation do not retrigger it.
The controls guide describes opt-in Find My handling and re-enabling alerts.

During the alarm, music continues advancing at reduced gain (about 21 dB lower),
then returns to its prior level. Mute and master volume remain in control.
The bundled alarm peaks around -3 dBFS; start at a low volume.

### Replace the alarm

Use `/cyberputer/sfx/suspicious.wav`, separate from the playlist.
It must be mono signed 16-bit PCM WAV at 8 kHz, at most two seconds
(32,000 PCM bytes). Reboot after replacing it to reload the immutable 32 KB cache.
A failed load can be retried by disabling/re-enabling alerts; there is no chirp
fallback. The cache persists across menu exits for asynchronous playback safety.

## SD and memory behavior

Insert or remove media with music and logging stopped. A missing card needs to
be inserted before reboot; the player does not remount shared SD hardware.
Long SD stalls or heavy writes can cause audio gaps. Read failures display a
status instead of retrying indefinitely; playback/track selection can retry.

A low-priority worker owns audio file access and four fixed 2,048-sample buffers
(16,384 PCM bytes), with a 4 KB task stack and bounded filenames. UI updates do
not open, seek or read SD files. The worker cooperates with the logger mutex.
Music file size does not increase RAM use. Buffer ownership prevents refill
while the speaker retains pointers, including during asynchronous stop/skip.

## Recording credits

Source and license information lives with the recordings:
[music credits](../sdcard/cyberputer/music/CREDITS.md) and
[alarm credits](../sdcard/cyberputer/sfx/CREDITS.md).
