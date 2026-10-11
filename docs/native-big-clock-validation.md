# Native Big Clock validation — CB1b

October 11, 2026. Original session-only digital clock view over a shared application-owned UTC snapshot. The dashboard's existing timer remains the sole OS clock reader; its checked UTC civil value also feeds the modal. Synchronization evidence is Unknown. DE time, analog rendering, weather, alarms and countdown interaction remain unavailable in this view, with explicit labels rather than invented data or fallback.

The caller chooses explicit Digital/UTC/24-hour/date/seconds replacement defaults; these are not original HamClock defaults. Options stay in memory across closing/reopening and do not write station settings. The modal owns input copies, retains option failures across successful source refreshes, separates source freshness errors, blocks reentrant callbacks before sampling, and distinguishes preparation failure from postcommit Qt rendering limitations.

## Provisional findings and fixes

Root's first integrated test caught a compact layout regression when the entry was added to the sidebar. Moving it to the masthead and hiding the redundant Appearance caption only at compact width restored 800x480 support; the named selector, station panels and map remain. Independent source review found an overly broad preparation catch after value commit and synchronous callback reentry; these were corrected before the final source gate.

Actual native Light screenshots then revealed a black scroll-area canvas with dark time/status text despite a correctly Light outer frame. Explicit local viewport/content background and rendered cross-host probes corrected this; a rebuilt native observation confirmed the canvas fix. A subsequent native Options observation found invisible standard combo arrows and white-on-white checked indicators. Original local painted disclosure/check indicators and explicit dialog-resolved indicator canvas/ink corrected those defects. Final rebuilt native repeats described below verified checked and unchecked states in Light/Dark/System. These observations show why the provisional passing offscreen tests do not satisfy the native gate.

## Final executable and independent evidence

The final 317-check revision passed author and independent strict C++17 warnings/ASan/UBSan. The independent auditor also passed 25 hostile reentry/error/lifecycle probes and freshly configured/built all 26 CTest targets in 8.90 seconds. Root rebuilt the actual dashboard binary, passed all 26 CTest targets and make verify, then copied that final binary into the stopped disposable QA bundle and relaunched it. Earlier 180/254/299 passes are historical, not the final gate. Exact frozen source/test hashes are recorded in the independent audit.

## Actual final native observations

- Light canvas, time/date/zone/status and Options labels are legible. All three disclosure arrows and checked/unchecked checkbox interiors now remain visible on the Light host view; markers use explicit shapes as well as color. Dark and System (current dark OS appearance) repeats showed legible canvas, arrows, checked/unchecked markers and keyboard focus. OS appearance settings themselves were not changed.
- Keyboard Home on the focused Clock face selected Analog and displayed Analog unavailable; End restored Digital. Tab/Down selected DE and showed Time unavailable with the DE label and selected-zone status, without showing UTC instead. Home explicitly restored UTC.
- Hour format changed to 12-hour and displayed 10:25 PM; independently toggling date and seconds hid only the chosen fields. Checked and unchecked markers changed visibly, selected values appeared in the native accessibility tree, and the focused checkbox had a visible border.
- Opening the focused hour-format popup via Space and dismissing it with Escape worked. Its visual capture failed with ScreenCaptureKit stream error -3811; popup-row visual appearance therefore remains unverified natively. No denied computer surface was worked around. Closed controls and subsequent screenshots captured normally.
- Escape dismissed Options first and restored Options focus; a second Escape exited and restored the dashboard Big Clock entry. Keyboard Space reopened the modal with 12-hour/date/seconds choices retained. Subsequent source refreshes continued updating actual OS-derived UTC, with synchronization still explicitly Unknown.
- Dark mode seconds toggle restored HH:MM:SS PM with legible checked state. System reopened with the same session preferences. The 800x480 logical view and Options content fit the captured window, and the named masthead entry preserves station/map geometry. Enlarged-font layout is covered offscreen; actual enlarged-font native use remains untested.

These observations concern the exact rebuilt 317-check source. They do not certify a synchronized source, missing original features or platform-wide accessibility compliance.

## Limits

The host is macOS with Qt 6.11.2. Disposable synthetic station settings are used for QA, while displayed UTC comes from the operating-system clock; that source is not certified synchronized. Live HamClock/user station settings are untouched. Physical touch/Pi 3/Pi 4/seven-inch hardware, Qt 6.4 execution, screen-reader announcements, actual source-error injection, enlarged-font native interaction, full final-original feature parity and accessibility/legal compliance remain separate or untested gates. Unit fault/lifecycle and rendered contrast checks do not establish those results. No new dependency, external icon/data asset, radio operation or provider connection is introduced.
