#include "visualization_view.h"
#include "city_audio.h"
#include "core/visualization/controls.h"
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
#include <algorithm>

namespace VisualizationView {
namespace {
using namespace Visualization;
// The existing scan task is the sole radio owner. The UI requests transitions;
// it never starts/stops NimBLE or frees results under a running scanner.
std::atomic<bool> opened{false}, closing{false}, stopped{false}, ready{false};
std::atomic<bool> paused{false}, scanFailed{false};
std::atomic<bool> activeNames{false}, activeApplied{false};
std::atomic<uint8_t> soundEvents{0}; // Bit 0 other flags; bit 1 optional Find My.
bool restoreScan = false, displayOn = true, stats = false;
bool showHelp=false;
bool showFindings=true, helpReturnToMenu=false;
bool includeFindMy=false;
std::atomic<uint8_t> lastFlagPlatform{0};
uint32_t flagNoticeAt=0;
bool flagNotice=false;
uint32_t findMyNoticeAt=0;
bool findMyNotice=false;
size_t helpScroll=0;
uint32_t audioNoticeAt=0;
bool audioNotice=false;
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
uint32_t page=0,pageAt=0;
bool autoPage=true;

void copyObservations(uint32_t now) {
    portENTER_CRITICAL(&storeMux);
    store.expire(now);
    snapshot=store.snapshot();
    portEXIT_CRITICAL(&storeMux);
}
class ObservationCallbacks final : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* device) override {
        if(!device || closing.load() || device->getAddress().equals(NimBLEDevice::getAddress()))return;
        std::array<uint8_t,7> identity{};
        const auto address=device->getAddress();
        std::memcpy(identity.data(),address.getVal(),6);identity[6]=address.getType();
        const auto name=device->getName();
        using namespace DeviceClassifier;
        auto match=classifyName(name);
        for(uint8_t i=0;i<device->getServiceUUIDCount();++i)
            match=strongerMatch(match,classifyService(device->getServiceUUID(i).toString()));
        for(uint8_t i=0;i<device->getServiceDataCount();++i){
            const auto data=device->getServiceData(i);
            match=strongerMatch(match,classifyServiceData(device->getServiceDataUUID(i).toString(),reinterpret_cast<const uint8_t*>(data.data()),data.size()));
        }
        for(uint8_t i=0;i<device->getManufacturerDataCount();++i){
            const auto data=device->getManufacturerData(i);
            match=strongerMatch(match,classifyManufacturer(reinterpret_cast<const uint8_t*>(data.data()),data.size()));
        }
        const uint32_t now=millis();
        portENTER_CRITICAL(&storeMux);
        const auto event=store.observe(identity,name.data(),name.size(),device->getRSSI(),now,match);
        portEXIT_CRITICAL(&storeMux);
        if(event==ObservationEvent::Flagged){
            lastFlagPlatform.store(uint8_t(match.platform));
            soundEvents.fetch_or(match.platform==Platform::AppleFindMy?2:1);
        }
    }
} observationCallbacks;
} // namespace

bool isOpen() { return opened.load(); }
bool setMode(Visualization::Mode mode) {
    const auto* next=Visualization::rendererFor(mode);
    if (!next) return false;
    renderer=next; // Every mode reuses the same surface and observation table.
    page=0;pageAt=millis();autoPage=true;
    return true;
}
static bool openImpl(Visualization::Mode mode,bool menuHelp) {
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
    closing=false;stopped=false;ready=false;paused=menuHelp;scanFailed=false;
    activeNames=false;activeApplied=false;soundEvents=0;showHelp=menuHelp;
    helpReturnToMenu=menuHelp;helpScroll=0;showFindings=true;
    includeFindMy=false;findMyNotice=false;flagNotice=false;
    if(!menuHelp)CityAudio::begin();
    stats=false;lastFrame=0;framePeriod=50;
    displayOn=true;NetworkContext::displayEnabled=true;M5.Lcd.wakeup();
    UIContext::visualizationActive=true;
    MenuController::closeSilent();
    opened=true; // Publish fully initialized state to ScanTask last.
    return true;
}
bool open(Visualization::Mode mode) { return openImpl(mode,false); }
bool openHelp() {
    if(isOpen()){
        showHelp=true;helpScroll=0;
        displayOn=true;NetworkContext::displayEnabled=true;M5.Lcd.wakeup();
        return true;
    }
    return openImpl(Visualization::Mode::City,true);
}
void close() { if(isOpen()){CityAudio::end();closing=true;} }

bool serviceScanner() {
    if(!isOpen())return false;
    auto* scan=NimBLEDevice::getScan();
    if(closing.load()) {
        if(configured && scan){scan->clearResults();scan->setScanCallbacks(nullptr);scan->setMaxResults(255);scan->setScanResponseTimeout(10240);}
        configured=false;
        ScanContext::scanCancelRequested=false;
        stopped=true; // All radio access/cleanup finishes before the UI exits.
        return true;
    }
    if(!scan){scanFailed=true;ready=true;return true;}
    if(!configured){
        scan->clearResults();scan->setMaxResults(0);
        scan->setScanCallbacks(&observationCallbacks,false);
        scan->setScanResponseTimeout(150);
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
    // Callback-only delivery prevents the first anonymous arrivals filling a
    // retained result list before later named advertisements can be considered.
    if(!scan->start(1000)) {scanFailed=true;return true;}
    scanFailed=false;
    while(scan->isScanning()){
        if(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)<32*1024){scan->stop();scanFailed=true;}
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    scan->clearResults();
    return true;
}
void handleKey(char key) {
    if(closing.load())return;
    const auto action=actionFor(key);
    if(showHelp){
        if(action==Action::Up && helpScroll)--helpScroll;
        if(action==Action::Down && helpScroll<helpMaxScroll())++helpScroll;
        if(action==Action::Left)helpScroll=helpScroll>HELP_ROWS?helpScroll-HELP_ROWS:0;
        if(action==Action::Right)helpScroll=std::min(helpMaxScroll(),helpScroll+HELP_ROWS);
        if(action==Action::Help || action==Action::Menu){
            if(helpReturnToMenu)close();else showHelp=false;
        }
        if(action==Action::Mute)CityAudio::mute();
        return;
    }
    if(action==Action::Menu){close();return;}
    if(action==Action::Pause)paused=!paused.load();
    if(action==Action::City)setMode(Mode::City);
    if(action==Action::Radar)setMode(Mode::Radar);
    if(action==Action::Rain)setMode(Mode::Rain);
    if(action==Action::Odyssey)setMode(Mode::Odyssey);
    if(action==Action::Left || action==Action::Right){autoPage=false;page+=action==Action::Right?1:(page?uint32_t(-1):0);}
    if(action==Action::AutoPage){autoPage=true;pageAt=millis();}
    if(action==Action::Active)activeNames=!activeNames.load();
    if(action==Action::Music){CityAudio::toggleMusic(millis());audioNotice=true;audioNoticeAt=millis();}
    if(action==Action::NextTrack){CityAudio::nextTrack();audioNotice=true;audioNoticeAt=millis();}
    if(action==Action::Alerts){
        CityAudio::toggleEffects();
        // Enabling alerts can be tested with an already visible Flipper.
        if(CityAudio::effectsEnabled())for(const auto& e:snapshot)
            if(e.used && uint32_t(millis()-e.lastSeen)<EXPIRE_MS && DeviceClassifier::isFlagged(e.classification,includeFindMy)){
                CityAudio::notify(true,e.classification.platform==DeviceClassifier::Platform::AppleFindMy);
            }
    }
    if(action==Action::Mute)CityAudio::mute();
    if(action==Action::VolumeDown)CityAudio::adjustVolume(-16);
    if(action==Action::VolumeUp)CityAudio::adjustVolume(16);
    if(action==Action::Help)openHelp();
    if(action==Action::Stats)stats=!stats;
    if(action==Action::Findings)showFindings=!showFindings;
    if(action==Action::FindMy){
        includeFindMy=!includeFindMy;
        portENTER_CRITICAL(&storeMux);store.setFindMyEnabled(includeFindMy);portEXIT_CRITICAL(&storeMux);
        page=0;pageAt=millis();findMyNotice=true;findMyNoticeAt=millis();
        if(!includeFindMy){
            soundEvents.fetch_and(uint8_t(~2));CityAudio::cancelFindMyAlert();
        }else for(const auto& e:snapshot){
            if(e.used && uint32_t(millis()-e.lastSeen)<EXPIRE_MS && e.classification.platform==DeviceClassifier::Platform::AppleFindMy){
                CityAudio::notify(true,true);break;
            }
        }
    }
    if(action==Action::Display){
        displayOn=!displayOn;
        NetworkContext::displayEnabled=displayOn;
        if(displayOn)M5.Lcd.wakeup();else M5.Lcd.sleep();
    }
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
    if(autoPage && !showHelp && uint32_t(now-pageAt)>=6000){++page;pageAt=now;}
    // Audio timing remains independent of redraw cadence and display sleep.
    const uint8_t events=soundEvents.exchange(0);
    // Defensive final gate, also discarding delayed Find My audio requests.
    if(!includeFindMy)CityAudio::cancelFindMyAlert();
    if((events&1) || ((events&2) && includeFindMy)){flagNotice=true;flagNoticeAt=now;}
    if(events&1)CityAudio::notify(true);
    if((events&2) && includeFindMy)CityAudio::notify(true,true);
    if(!closing.load() && !helpReturnToMenu)CityAudio::update(now,ready.load());
    if(!displayOn || uint32_t(now-lastFrame)<framePeriod)return;
    lastFrame=now;
    copyObservations(now);
    const char* status=nullptr;char metrics[40];
    if(closing.load())status="RETURNING TO MENU...";
    else if(!ready.load())status="WAITING FOR SCAN TO YIELD";
    else if(scanFailed.load())status="SCAN ERROR / RETRYING";
    else if(stats){
        std::snprintf(metrics,sizeof(metrics),"%lu MS / %u KB / %s",
                      static_cast<unsigned long>(lastCostUs/1000),unsigned(ESP.getFreeHeap()/1024),
                      paused.load()?"PAUSED":"RUNNING");
        status=metrics;
    }
    else if(paused.load())status="SCAN PAUSED / P TO RESUME";
    else if(activeNames.load()!=activeApplied.load())status="NAME MODE CHANGES NEXT SCAN";
    else if(findMyNotice && uint32_t(now-findMyNoticeAt)<3000)
        status=includeFindMy?"G FIND MY FLAGS ON":"G FIND MY INFORMATIONAL ONLY";
    else if(flagNotice && uint32_t(now-flagNoticeAt)<3000){
        std::snprintf(metrics,sizeof(metrics),"FLAG %s",DeviceClassifier::label(DeviceClassifier::Platform(lastFlagPlatform.load())));
        status=metrics;
    }
    else if(CityAudio::effectsEnabled() && std::strcmp(CityAudio::alertStatus(),"ALERT READY")!=0)
        status=CityAudio::alertStatus();
    else if(audioNotice && uint32_t(now-audioNoticeAt)<6000)
        status=(uint32_t(now-audioNoticeAt)/2000)%2?CityAudio::trackName():CityAudio::musicStatus();
    else if(CityAudio::musicEnabled() && std::strcmp(CityAudio::musicStatus(),"PLAYING SD")!=0)
        status=CityAudio::musicStatus();
    else {
        std::snprintf(metrics,sizeof(metrics),"A %s B %s F %s G %s V%03u %s",
            activeApplied.load()?"ACT":"PAS",CityAudio::musicEnabled()?"ON":"OFF",
            CityAudio::effectsEnabled()?"ON":"OFF",includeFindMy?"ON":"OFF",unsigned(CityAudio::volume()),
            MenuController::getAudioEnabled()?"H":"MUTE");
        status=metrics;
        if(renderer->mode==Mode::Odyssey)status=nullptr;
    }
    const uint32_t start=micros();
    if(showHelp && !closing.load())drawHelp(surface,helpScroll);
    else renderer->draw(surface,{snapshot,now,status,page,showFindings,includeFindMy});
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
