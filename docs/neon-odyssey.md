# Scene 4: Neon Odyssey

Version 0.4.0 adds a 54-second eye-candy suite: 18 seconds each of twisting
octagonal tunnel, rotating circuit grid, and a radial warp field. Slow plasma
underlays all three. LABScon branding stays small; device names, counts, and
pagination are deliberately absent. Scanning, suspicious alerts, existing SD
music and global controls continue. Status/errors and S stats can still overlay
the bottom. T has no visual effect here. Switch with 1–4 or the menu.

## Research and design (2026-09-13)

The strongest fits for this CPU-only 240×135 display are classic tunnel,
plasma, rotozoom, and starfield techniques. This is a suitability assessment,
not a claim of an objective demoscene ranking.

- [Antonin Carette's tunnel write-up](https://carette.xyz/posts/the_tunnel_effect_demoscene/)
  explains inverse-distance tunnel mapping and precomputation for CPU rendering.
  Our original renderer instead projects a small set of octagonal wire rings,
  keeping geometry and memory bounded without a full-screen UV table.
- [DOSFX's author-maintained effect collection](https://github.com/ponceto/dosfx)
  demonstrates plasma and rotozoom as old-school software effects. We combine
  these ideas in original code: separable sine waves and a transformed circuit
  pattern. No source code or artwork from that GPL project was copied.
- [Inigo Quilez's Rendering Worlds with Two Triangles](https://iquilezles.org/articles/nvscene2008/rwwtt.pdf)
  explores procedural rendering/raymarching. Full per-pixel iterative 3D is
  deferred here: predictable frame cost and BLE/audio headroom matter more.

The renderer uses the existing 16-color, 16,200-byte canvas, no additional heap
allocations, and 500 bytes of stack-local plasma wave samples. Plasma/circuit
sampling is 80×45; line detail is native resolution. Animation is time-driven,
not audio-reactive. Effects cut every 18 seconds without white flash transitions.
Hardware frame time must be checked with S; host tests cannot validate ESP32 FPS.

## Versioning and reported Find My alerts

`firmware/src/config/version.h` is the release version source, visible in menu
and H help. Increment minor for each new feature release and patch for fixes;
do not reuse an already distributed version. 0.4.0 begins explicit numbering.

Apple Find My remains informational with G off, including on each scene entry.
The reported default-on alert has not been reproduced in host tests or verified
on hardware. A final audio gate now clears delayed Apple-only requests while G
is off. Newly flagged scanner events briefly show their platform in the footer
(`FLAG ...`); this indicates a detection, not proof an audio sample played.
Google Find Hub, Tile, and SmartTag retain their separate existing flag policies.

To diagnose a recurrence, note the menu version, G state, exact FLAG label,
and whether the event was audible or just an informational device label.
