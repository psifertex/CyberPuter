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
| S | Pause/resume scanning |
| B | Toggle SD music (on by default) |
| N | Next music track; also selects while music is off |
| F | Toggle suspicious-device alerts (on by default) |
| X | Mute music and alerts; B/F enable them again |
| - | Volume down 16, minimum 0 |
| = or + | Volume up 16, maximum 160 |
| D | Sleep/wake display; audio/scanning continue |
| P | Toggle frame-time/free-heap display |
| H | Toggle cycling footer key reference |
| Backtick/Esc, M, Q | Leave visualization and open menu |

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
| Legacy help overlay | Any key | Dismiss help |

M/Q and the visualization number shortcuts are not menu navigation shortcuts.
Old mascot-home shortcuts still present in upstream source are no longer reachable
as a home screen and are not part of this map.
