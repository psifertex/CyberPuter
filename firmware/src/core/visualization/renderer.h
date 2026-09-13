#pragma once
#include "observations.h"

namespace Visualization {
constexpr int WIDTH = 240;
constexpr int HEIGHT = 135;
constexpr size_t FRAME_BYTES = WIDTH * HEIGHT / 2;
enum Color : uint8_t { Background, Building, Dim, Grid, VioletDim, TealDim,
                      Violet, Cyan, Pink, White, Amber, Rain, Road, Window, Sign, Black };
extern const uint32_t PALETTE[16];
// One surface shared by every renderer; no per-mode sprite or observation copy.
struct Surface {
    virtual ~Surface() = default;
    virtual void fill(int x, int y, int width, int height, uint8_t color) = 0;
};
enum class Mode : uint8_t { City, Radar, Rain };
struct Frame {
    const Snapshot& observations;
    uint32_t now;
    const char* status; // Optional lifecycle/error/paused message in the footer.
    uint32_t page = 0; // Shared automatic/manual page cursor across modes.
};
struct Renderer {
    Mode mode;
    const char* name;
    void (*draw)(Surface&, const Frame&);
};
// All modes reuse the same surface, palette and observation snapshot.
const Renderer* rendererFor(Mode mode);
void drawCity(Surface& surface, const Frame& frame);
void drawRadar(Surface& surface, const Frame& frame);
void drawRain(Surface& surface, const Frame& frame);
} // namespace Visualization
