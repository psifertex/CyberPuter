#pragma once
#include "core/visualization/renderer.h"

namespace VisualizationView {
// UI-thread API. Opening is asynchronous while the existing scan finishes.
bool open(Visualization::Mode mode = Visualization::Mode::City);
bool isOpen();
bool openHelp();
void close();
void update();
void handleKey(char key);
bool setMode(Visualization::Mode mode);
// Called only by the existing ScanTask; returns true when it owns this tick.
bool serviceScanner();
} // namespace VisualizationView
