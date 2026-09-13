#include "visualization_view.h"
#include "city_audio.h"
#include "app/context/ui_context.h"
#include "app/context/scan_context.h"
#include "app/context/network_context.h"
#include "infrastructure/platform/hardware.h"
#include "ui/menu/menu_controller.h"
#include <NimBLEDevice.h>
#include <esp_heap_caps.h>
#include <atomic>
#include <cstdio>
#include <cstring>

namespace VisualizationView {
namespace {
using namespace Visualization;
// The existing scan task is the sole radio owner. The UI requests transitions;
// it never starts/stops NimBLE or frees results under a running scanner.
std::atomic<bool> opened{false}, closing{false}, stopped{false}, ready{false};
std::atomic<bool> paused{false}, scanFailed{false};
std::atomic<bool> activeNames{false}, activeApplied{false};
std::atomic<uint8_t> soundEvents{0}; // Coalesced discoveries/name resolutions.
bool restoreScan = false, displayOn = true, stats = false;
bool showHelp=false;
bool configured = false; // ScanTask only.
const Renderer* renderer = rendererFor(Mode::City); // UI only.
ObservationStore store;
Snapshot snapshot;
portMUX_TYPE storeMux = portMUX_INITIALIZER_UNLOCKED;
M5Canvas canvas(&M5.Lcd);
struct CanvasSurface final : Surface {
    void fill(int x,int y,int w,int h,uint8_t color) override {
        canvas.fillRect(x,y,w,h,color);
    }
} surface;
uint32_t lastFrame=0, framePeriod=50, lastCostUs=0;

void copyObservations(uint32_t now) {
    portENTER_CRITICAL(&storeMux);
    store.expire(now);
    snapshot=store.snapshot();
    portEXIT_CRITICAL(&storeMux);
}
} // namespace

bool isOpen() { return opened.load(); }
bool setMode(Visualization::Mode mode) {
    const auto* next=Visualization::rendererFor(mode);
    if (!next) return false;
    renderer=next; // Every mode reuses the same surface and observation table.
    return true;
}
bool open(Visualization::Mode mode) {
    if (isOpen()) return setMode(mode);
    if (!setMode(mode)) return false;
    UIContext::DisplayGuard displayGuard(true);
    if (!displayGuard) return false;
    // Cardputer ADV has no PSRAM. A 4-bit indexed sprite is 16,200 bytes.
    // Leave 48 KiB free for BLE, other tasks and temporary scan results.
    constexpr size_t RESERVE=48*1024;
    if (heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<FRAME_BYTES+RESERVE ||
        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<FRAME_BYTES+1024) {
        Serial.println("City: insufficient internal heap; keeping current view");
        return false;
    }
    canvas.setPsram(false);
    canvas.setColorDepth(4);
    if (!canvas.createSprite(WIDTH,HEIGHT)) {
        canvas.deleteSprite();
        Serial.println("City: sprite allocation failed; keeping current view");
        return false;
    }
    for(size_t i=0;i<16;++i)canvas.setPaletteColor(i,PALETTE[i]);
    portENTER_CRITICAL(&storeMux);store.clear();portEXIT_CRITICAL(&storeMux);
    snapshot={};
    restoreScan=ScanContext::bleScanEnabled.exchange(false);
    ScanContext::scanCancelRequested.store(true);
    closing=false;stopped=false;ready=false;paused=false;scanFailed=false;
    activeNames=false;activeApplied=false;soundEvents=0;showHelp=false;
    CityAudio::begin();
    stats=false;lastFrame=0;framePeriod=50;
    displayOn=true;NetworkContext::displayEnabled=true;M5.Lcd.wakeup();
    UIContext::visualizationActive=true;
    MenuController::closeSilent();
    opened=true; // Publish fully initialized state to ScanTask last.
    return true;
}
void close() { if(isOpen()){CityAudio::end();closing=true;} }

bool serviceScanner() {
    if(!isOpen())return false;
    auto* scan=NimBLEDevice::getScan();
    if(closing.load()) {
        if(configured && scan){scan->clearResults();scan->setMaxResults(255);}
        configured=false;
        ScanContext::scanCancelRequested=false;
        stopped=true; // All radio access/cleanup finishes before the UI exits.
        return true;
    }
    if(!scan){scanFailed=true;ready=true;return true;}
    if(!configured){
        scan->clearResults();scan->setMaxResults(MAX_OBSERVATIONS);
        // Both scan modes avoid pairing, GATT and connections. Active mode
        // requests scan responses, which may contain an advertised local name.
        scan->setPhy(NimBLEScan::Phy::SCAN_1M);
        scan->setInterval(100);scan->setWindow(60);
        configured=true;
    }
    ready=true;
    if(paused.load())return true;
    const bool useActive=activeNames.load();
    scan->setActiveScan(useActive); // Only change settings between windows.
    activeApplied=useActive;
    // One-second windows bound close/pause latency after the legacy scan yields.
    // The retained NimBLE list and the observation table are both capped at 24.
    if(!scan->start(1000)) {scanFailed=true;return true;}
    scanFailed=false;
    while(scan->isScanning())vTaskDelay(pdMS_TO_TICKS(20));
    const auto results=scan->getResults();
    for(int i=0;i<results.getCount();++i){
        const auto* device=results.getDevice(i);
        if(!device || device->getAddress().equals(NimBLEDevice::getAddress()))continue;
        std::array<uint8_t,7> identity{};
        const auto address=device->getAddress();
        std::memcpy(identity.data(),address.getVal(),6);identity[6]=address.getType();
        const auto name=device->getName();
        const uint32_t now=millis();
        portENTER_CRITICAL(&storeMux);
        const auto event=store.observe(identity,name.data(),name.size(),device->getRSSI(),now);
        portEXIT_CRITICAL(&storeMux);
        if(event==ObservationEvent::NameResolved)soundEvents.fetch_or(2);
        else if(event==ObservationEvent::Discovered)soundEvents.fetch_or(1);
    }
    scan->clearResults();
    return true;
}
void handleKey(char key) {
    if(closing.load())return;
    if(key=='`'||key=='m'||key=='M'||key=='q'||key=='Q'){close();return;}
    if(key=='s'||key=='S')paused=!paused.load();
    if(key=='a'||key=='A')activeNames=!activeNames.load();
    if(key=='b'||key=='B')CityAudio::toggleMusic(millis());
    if(key=='f'||key=='F')CityAudio::toggleEffects();
    if(key=='x'||key=='X')CityAudio::mute();
    if(key=='-')CityAudio::adjustVolume(-16);
    if(key=='='||key=='+')CityAudio::adjustVolume(16);
    if(key=='h'||key=='H')showHelp=!showHelp;
    if(key=='p'||key=='P')stats=!stats;
    if(key=='d'||key=='D'){
        displayOn=!displayOn;
        NetworkContext::displayEnabled=displayOn;
        if(displayOn)M5.Lcd.wakeup();else M5.Lcd.sleep();
    }
    // TODO: Bind mode cycling here once Radar and Rain have measured budgets.
}
void update() {
    if(!isOpen())return;
    if(closing.load() && stopped.load()){
        UIContext::DisplayGuard displayGuard(true);
        if (!displayGuard) return;
        canvas.deleteSprite(); // Release only after the scan task acknowledges.
        UIContext::visualizationActive=false;
        opened=false;
        // Reopen the menu before the previous scan mode can draw again.
        displayOn=true;NetworkContext::displayEnabled=true;M5.Lcd.wakeup();
        MenuController::open();
        ScanContext::bleScanEnabled=restoreScan;
        return;
    }
    const uint32_t now=millis();
    // Audio timing remains independent of redraw cadence and display sleep.
    const uint8_t events=soundEvents.exchange(0);
    if(events)CityAudio::notify(events&2);
    if(!closing.load())CityAudio::update(now,ready.load());
    if(!displayOn || uint32_t(now-lastFrame)<framePeriod)return;
    lastFrame=now;
    copyObservations(now);
    const char* status=nullptr;char metrics[40];
    if(closing.load())status="RETURNING TO MENU...";
    else if(!ready.load())status="WAITING FOR SCAN TO YIELD";
    else if(scanFailed.load())status="SCAN ERROR / RETRYING";
    else if(paused.load())status="SCAN PAUSED / S TO RESUME";
    else if(activeNames.load()!=activeApplied.load())status="NAME MODE CHANGES NEXT SCAN";
    else if(showHelp)status="A NAMES B MUSIC F FX X MUTE -/=";
    else if(stats){
        std::snprintf(metrics,sizeof(metrics),"%lu MS / %u KB / ESC BACK",
                      static_cast<unsigned long>(lastCostUs/1000),unsigned(ESP.getFreeHeap()/1024));
        status=metrics;
    }
    else {
        std::snprintf(metrics,sizeof(metrics),"A %s B %s F %s V%03u %s",
            activeApplied.load()?"ACT":"PAS",CityAudio::musicEnabled()?"ON":"OFF",
            CityAudio::effectsEnabled()?"ON":"OFF",unsigned(CityAudio::volume()),
            MenuController::getAudioEnabled()?"H HELP":"MASTER MUTE");
        status=metrics;
    }
    const uint32_t start=micros();
    renderer->draw(surface,{snapshot,now,status});
    {
        UIContext::DisplayGuard displayGuard(true);
        if (!displayGuard) return;
        canvas.pushSprite(0,0);
    }
    lastCostUs=micros()-start;
    // If a frame costs most of its budget, fall back from 20 to 10 FPS.
    // Never allocate a second buffer or let rendering monopolize the UI loop.
    if(lastCostUs>40000)framePeriod=100;
}
} // namespace VisualizationView
