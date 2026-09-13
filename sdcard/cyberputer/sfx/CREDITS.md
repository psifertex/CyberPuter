# Suspicious-device alert

`suspicious.wav`: **Alarm** by **EZduzziteh**, released under **CC0 1.0**.

- Artist's upload and license: https://opengameart.org/content/alarm-1
- Original: https://opengameart.org/sites/default/files/alarm_2.ogg
- License: https://creativecommons.org/publicdomain/zero/1.0/
- Legal code: https://creativecommons.org/publicdomain/zero/1.0/legalcode

Complete 1.846-second sci-fi game alarm converted from stereo Ogg to mono
signed 16-bit PCM WAV at 8,000 Hz, with short edge fades and approximately
-3 dBFS peak normalization. The original quiet conversion receives +21.6 dB;
measured peak is -3.0 dBFS and mean level is -6.7 dBFS, without clipping.
The 29,536 PCM bytes fit the player's fixed 32 KB cache. Converted recording
remains CC0. Attribution is appreciated but not required. No endorsement implied.
Source/license verified September 12, 2026.

Original SHA-256:
`ace7c78d3e071eadce6db3df560fb4722af41efbc0934b55439dcc28ab27bb20`

Converted SHA-256:
`2476050c3b6cefdaaee1a6e387ce854704a6d0a3f7977bbed04f094351d6bbe8`

Reproduce with ffmpeg (refuses to overwrite):

```sh
ffmpeg -v error -n -i alarm_2.ogg -map_metadata -1 -ac 1 -ar 8000 \
  -af 'volume=0.65,afade=t=in:d=0.008,afade=t=out:st=1.830:d=0.016,volume=21.6dB' \
  -c:a pcm_s16le suspicious.wav
```

Copy this directory to `/cyberputer/sfx` on the card. Keep effects outside
`/cyberputer/music` so they never enter the music playlist.
