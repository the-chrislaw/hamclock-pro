# Native countdown validation — CU2

Validated October 11, 2026 on macOS with installed Qt 6.11.2. Root tested the rebuilt final binary through native computer controls in build/FilledMapQA.app with disposable synthetic N1TEST station settings. Live HamClock and user station settings were untouched.

## Automated and independent evidence

Root final CMake build and all 22 CTest targets passed; make verify passed. An independent agent reviewed actual final dialog and dashboard integration, reran 97 permanent and 26 hostile checks with strict warnings and ASan/UBSan, and independently configured/built all 22 CTest targets (5.85 seconds). Exact frozen dialog hashes and audit scope are in workstreams/native-countdown-dialog-audit.md. No new dependencies, downloaded assets, network operations, radio operations or persistence are introduced by this increment.

## Actual final native observations

After copying the final rebuilt binary and relaunching the disposable QA application:

- Dashboard Countdown entry and existing station/map/appearance/stopwatch controls fit the 800 by 480 logical dashboard. Countdown dialog controls and session-only explanation fit without clipping.
- Light shows a legible light duration field with dark numerals, visible focus and explicit large minus/plus controls. Dark shows legible light numerals and controls on dark surfaces. System follows the current dark host appearance. Host appearance switching itself was not changed or tested through macOS settings.
- Deleting the draft then clicking Set & reset shows a readable error while Ready/configured one minute remain unchanged. The error survived a later keyboard-navigation observation after timer refreshes. Keyboard Space on plus repaired the draft to two minutes and cleared the error without changing configured duration.
- Set & reset applied two minutes; Start changed status to Running. Pause, Exit and keyboard Space reopening preserved Paused with 1:57 remaining and two-minute configuration. Resume changed status back to Running. Exit returned keyboard focus to the dashboard Countdown entry.
- Reset followed by draft minus, Set & reset and Start configured a fresh one-minute run at approximately 21:43:56 UTC. Closing and reopening while previewing Dark and System preserved the running session (52 then 43 seconds observed). Reopened near the end and observed six seconds, closed again at 21:44:53 UTC, then reopened after the deadline: zero remaining, Time elapsed - alert pending, and enabled Dismiss alert. Dismiss changed status to Time elapsed and disabled itself while retaining zero/configured one minute. Restart returned Running at one full minute; Reset and Exit returned to the dashboard Countdown focus. These were actual real-time native observations, not injected unit-test timestamps.

## Findings fixed before publication

Successful refreshes previously erased action errors too quickly. Separate retained plain-text error state and multi-tick tests corrected this. Actual native Light originally inherited a dark spinbox field and tiny low-contrast arrows despite offscreen tests. Local semantic input styling and explicit 44-logical-pixel draft-only minus/plus controls corrected it; final Light/Dark/System native observations above repeated the gate.

## Limits and remaining work

Native screen-reader announcements, physical touch, seven-inch Pi screen, Pi 3/Pi 4, Qt 6.4 minimum, suspend/resume behavior and injected clock-failure presentation were not tested on actual hardware. Offscreen fault tests cover clock failures and stale dismissal tokens; they do not replace these native/hardware checks. This is session-only countdown control, with an explicit one-minute replacement default, not an assertion of original defaults, full HamClock parity, alarm/sound/GPIO support or accessibility/legal compliance. Big-clock preference work remains a separate unintegrated batch.
