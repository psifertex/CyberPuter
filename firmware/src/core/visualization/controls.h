#pragma once
#include <cstddef>
namespace Visualization {
enum class Action { None, City, Radar, Rain, Odyssey, Left, Right, Up, Down, AutoPage,
    Active, Pause, Music, NextTrack, Alerts, Mute, VolumeDown, VolumeUp,
    Display, Stats, Help, Findings, FindMy, Menu };
struct Binding { char key; bool fn; Action action; const char* help; };
// The dispatcher and both help entry points use this one binding table.
inline constexpr Binding BINDINGS[]={
    {'1',false,Action::City,"1          NEON CITY"},
    {'2',false,Action::Radar,"2          NEON RADAR"},
    {'3',false,Action::Rain,"3          SIGNAL RAIN"},
    {'4',false,Action::Odyssey,"4          NEON ODYSSEY"},
    {',',false,Action::Left,"LEFT       PREVIOUS PAGE"},
    {'/',false,Action::Right,"RIGHT      NEXT PAGE"},
    {';',false,Action::Up,"UP         SCROLL HELP UP"},
    {'.',false,Action::Down,"DOWN       SCROLL HELP DOWN"},
    {'0',false,Action::AutoPage,"0          AUTOMATIC PAGING"},
    {'t',false,Action::Findings,"T          SHOW/HIDE FINDINGS"},
    {'p',false,Action::Pause,"P          PAUSE/RESUME SCAN"},
    {'s',false,Action::Stats,"S          FRAME/HEAP STATS"},
    {'a',false,Action::Active,"A          ACTIVE NAME SCAN"},
    {'b',false,Action::Music,"B          MUSIC ON/OFF"},
    {'n',false,Action::NextTrack,"N          NEXT MUSIC TRACK"},
    {'f',false,Action::Alerts,"F          SUSPICIOUS ALERTS"},
    {'g',false,Action::FindMy,"G          FIND MY FLAGS ON/OFF"},
    {'m',false,Action::Mute,"M / X      MUTE MUSIC + ALERTS"},
    {'x',false,Action::Mute,nullptr},
    {'-',false,Action::VolumeDown,"-          VOLUME DOWN"},
    {'=',false,Action::VolumeUp,"= / +      VOLUME UP"},
    {'+',false,Action::VolumeUp,nullptr},
    {'d',false,Action::Display,"D          DISPLAY SLEEP/WAKE"},
    {'h',false,Action::Help,"H          OPEN/CLOSE HELP"},
    {'`',false,Action::Menu,"ESC / Q    BACK TO MENU"},
    {'q',false,Action::Menu,nullptr},
};
inline constexpr const char* HELP_NOTES[]={
    "MENU / DEVICE LISTS",
    "UP/DOWN    SELECT ITEM",
    "LEFT/RIGHT ADJUST MENU SLIDER",
    "ENTER      ACTION / CONFIRM",
    "ESC        BACK / CANCEL DELETE",
    "F          REFRESH FINDER LIST",
    "HELP       M/X STILL MUTES AUDIO",
};
inline Action actionFor(char key,bool fn=false) {
    if(key>='A' && key<='Z')key+=32;
    for(const auto& b:BINDINGS)if(b.key==key && b.fn==fn)return b.action;
    return Action::None;
}
inline size_t helpLineCount() {
    size_t n=sizeof(HELP_NOTES)/sizeof(*HELP_NOTES);
    for (const auto& b : BINDINGS) {
        if (b.help) ++n;
    }
    return n;
}
inline const char* helpLine(size_t index) {
    for(const auto& b:BINDINGS)if(b.help){if(!index--)return b.help;}
    if(index<sizeof(HELP_NOTES)/sizeof(*HELP_NOTES))return HELP_NOTES[index];
    return "";
}
inline constexpr size_t HELP_ROWS=9;
inline size_t helpMaxScroll() { return helpLineCount()>HELP_ROWS?helpLineCount()-HELP_ROWS:0; }
} // namespace Visualization
