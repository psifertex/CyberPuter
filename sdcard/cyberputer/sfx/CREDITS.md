# Suspicious-device alert

`suspicious.wav`: **Alarm** by **EZduzziteh**, released under **CC0 1.0**.

- Artist's upload and license: https://opengameart.org/content/alarm-1
- Original: https://opengameart.org/sites/default/files/alarm_2.ogg
- License: https://creativecommons.org/publicdomain/zero/1.0/
- Legal code: https://creativecommons.org/publicdomain/zero/1.0/legalcode

Complete 1.846-second sci-fi game alarm converted from stereo Ogg to mono
signed 16-bit PCM WAV at 8,000 Hz, gain 0.65, with short edge fades.
The 29,536 PCM bytes fit the player's fixed 32 KB cache. Converted recording
remains CC0. Attribution is appreciated but not required. No endorsement implied.
Source/license verified September 12, 2026.

Original SHA-256:
`ace7c78d3e071eadce6db3df560fb4722af41efbc0934b55439dcc28ab27bb20`

Converted SHA-256:
`630840c8ebdfa9684e0b4ace776a781c1b243a83648ac486fd8a6c9534771aa9`

Reproduce with ffmpeg (refuses to overwrite):

```sh
ffmpeg -v error -n -i alarm_2.ogg -map_metadata -1 -ac 1 -ar 8000 \
  -af 'volume=0.65,afade=t=in:d=0.008,afade=t=out:st=1.830:d=0.016' \
  -c:a pcm_s16le suspicious.wav
```

Copy this directory to `/cyberputer/sfx` on the card. Keep effects outside
`/cyberputer/music` so they never enter the music playlist.
