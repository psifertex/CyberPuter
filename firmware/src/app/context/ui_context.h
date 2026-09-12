#pragma once

#include <Arduino.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// ============================================================
//  UIContext — NibBLEs-Animationen, Overlays, Display-State
//
//  Thread-safety:
//    - std::atomic<bool>  → set FreeRTOS-Tasks (Core 1)
//                           this Flags read, loop() (Core 0)
//    - plain field        → only from loop() / use setup()
//    - taskMutex          → needs to be set before every 
//                           TaskHandle access
// ============================================================

namespace UIContext {

// City/radar/rain own the LCD while their modal view is active.
extern std::atomic<bool> visualizationActive;
extern SemaphoreHandle_t displayMutex;

// Serializes legacy animation primitives with visualization handoff and pushes.
// Recursive because legacy draw helpers call one another. Re-check ownership
// after taking the lock so a waiting mascot task cannot overwrite the city.
class DisplayGuard {
public:
    explicit DisplayGuard(bool visualization = false) {
        if (!displayMutex) { allowed = !visualization; return; }
        if (!visualization && visualizationActive.load()) return;
        locked = xSemaphoreTakeRecursive(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE;
        allowed = locked && (visualization || !visualizationActive.load());
    }
    ~DisplayGuard() { if (locked) xSemaphoreGiveRecursive(displayMutex); }
    explicit operator bool() const { return allowed; }
    DisplayGuard(const DisplayGuard&) = delete;
    DisplayGuard& operator=(const DisplayGuard&) = delete;
private:
    bool locked = false;
    bool allowed = false;
};

// ------------------------------------------------------------
//  FreeRTOS-Infrastructure
//  taskMutex safes all TaskHandle-Operations
// ------------------------------------------------------------
extern SemaphoreHandle_t taskMutex;

// ------------------------------------------------------------
//  Animations-Task-Handles
//  Before access use taskMutex!
// ------------------------------------------------------------
extern TaskHandle_t glassesTaskHandle;
extern TaskHandle_t angryTaskHandle;
extern TaskHandle_t happyTaskHandle;
extern TaskHandle_t sadTaskHandle;
extern TaskHandle_t thugLifeTaskHandle;

// ------------------------------------------------------------
//  Animations-Flags  (Core 1 write ↔ Core 0 read → atomic)
// ------------------------------------------------------------
extern std::atomic<bool> isGlassesTaskRunning;
extern std::atomic<bool> isAngryTaskRunning;
extern std::atomic<bool> isSadTaskRunning;
extern std::atomic<bool> isHappyTaskRunning;
extern std::atomic<bool> isThugLifeTaskRunning;
extern std::atomic<bool> isSpeechBubbleActive;

extern std::atomic<bool> isResearchModeActive;

// ------------------------------------------------------------
//  Overlay-State  (nur loop() read/write)
// ------------------------------------------------------------
extern bool helpOverlayVisible;

// ------------------------------------------------------------
//  Show Device-State  (only loop() read/write)
// ------------------------------------------------------------
extern std::atomic<bool> isChargingState;
extern std::atomic<int>  batteryPercent;

// ------------------------------------------------------------
//  Lifecycle-Helpers
// ------------------------------------------------------------

// Initialisations taskMutex. Called once in setup(),
// bevore any task is running.
void init();

// Gave true back if any Expression-Animation is running.
// Usefull if competing Animations-Tasks.
bool isAnyExpressionRunning();

void hideHelpOverlay();

// Stopps running Task gracefully (check Handle + Flag).
// expression: Pointer at atomic<bool>-Flag of Tasks
// handle:     Pointer at associated TaskHandle_t
void stopExpressionTask(std::atomic<bool>& runningFlag,
                        TaskHandle_t&      handle);

} // namespace UIContext
