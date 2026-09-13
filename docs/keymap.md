# Current Cardputer keyboard map

Applies to the latest source/build, including suspicious-only alerts. Alphabetic
bindings accept upper/lowercase. Punctuation is literal, without Fn. The four
arrow-marked punctuation keys are semicolon (up), period (down), comma (left),
and slash (right). Esc here means the backtick key.

## City, radar and signal rain

| Key | Action |
| --- | --- |
| 1 | Neon city |
| 2 | Neon radar |
| 3 | Signal rain |
| Comma | Previous device page; stop automatic paging |
| Slash | Next device page; stop automatic paging |
| 0 | Resume six-second automatic paging |
| A | Toggle active BLE name scanning (passive by default) |
| P | Pause/resume scanning |
| T | Hide/show findings without stopping scanning/audio |
| B | Toggle SD music (on by default) |
| N | Next music track; also selects while music is off |
| F | Toggle suspicious-device alerts (on by default) |
| M or X | Mute music and alerts; B/F enable them again |
| - | Volume down 16, minimum 0 |
| = or + | Volume up 16, maximum 160 |
| D | Sleep/wake display; audio/scanning continue |
| S | Toggle frame-time/free-heap display |
| H | Open/close full-screen help |
| Backtick/Esc, Q | Leave visualization and open menu; close help first |

In help, semicolon/period scroll one line and comma/slash scroll one page. No Fn
required. H, backtick/Esc or Q closes help; M/X still mutes audio. Other shortcuts
are inactive while reading help. The menu Show Help item opens the same page and
returns to the menu on close, without starting music/scanning.

Switching views preserves audio/scanning settings and resets paging to automatic
page one. Entering from the menu starts both audio options enabled again; the
saved master Audio setting and alarm-volume-derived initial volume still apply.
F on will alert for a currently visible flagged device, useful for testing with
your Flipper. Repeated advertisements do not continuously sound the alarm.

Other characters, Enter, Tab and standalone Fn have no visualization action.
There is no previous-track, screenshot, or dedicated test-sound binding.

## Main menu and utility screens

| Context | Keys | Action |
| --- | --- | --- |
| Main menu | Semicolon / period | Previous / next item |
| Main menu | Comma / slash | Adjust selected slider left / right |
| Main menu | Enter | Toggle setting or execute selected action |
| Main menu | Backtick/Esc | Return to city |
| Finder, connected-device, suspicious-device, file lists | Semicolon / period | Previous / next item |
| Lists | Enter | Execute selected item; confirms deletion when a file-delete prompt is open |
| Lists | Backtick/Esc | Go back; cancel a file-delete prompt first |
| Finder | F | Refresh device list |
| Approach view | Backtick/Esc | Go back; other keyboard inputs do nothing |
| Help page, including menu Help | Semicolon / period | Scroll up / down |
| Help page | Comma / slash | Scroll previous / next page |
| Help page | H, backtick/Esc, Q | Close help |
| Help page | M / X | Mute audio |

M/Q and the visualization number shortcuts are not menu navigation shortcuts.
Old mascot-home shortcuts are no longer reachable as a home screen. The old help
entry point now forwards to the new help page rather than listing those shortcuts.

## Research Mode setting

This inherited setting controls active BLE scanning in the legacy scanner
(`ble_scanner.cpp`). It also changes the legacy research icon/mascot dialogue.
Wardriving enables it automatically. It does not control the costume scanner:
use A in city/radar/rain for active scan requests, without pairing or GATT
connections. Research Mode is not a special mode for revealing encrypted names.
