# Agent log: ui

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (hourly run)
- Issue #80: added `--pane N=<widget id>` (repeatable) to choose pane contents for screenshots; unknown ids exit 2. CMake registers one `qml-pane-<id>` test per widget (800x480, fails on any QML warning), so every widget is now render-checked in CI.
- Not merged this run: #73 (docs, conflicts with main; not an agent/ui branch), #60 (area F, macOS/iPad checks cancelled).
- Next: A0 remainder, starting with removing the header bar on small screens and tight spacing at 1024x600, then the `compact` mechanism check in each card.

## 2026-10-13 (hourly run, 2)
- Merged #89 (docs, area G priorities). #73 still has merge conflicts and is not an agent/ui branch; left alone.
- `Theme.dense` (window <= 1280x720): no header bar, pad/gap/radius roughly halved, narrower station column, station clock fits its column, Settings button moved into the station card. Tap anywhere on a pane opens the chooser; horizontal swipe steps through widgets (including empty). Checked by screenshots at 800x480, 1024x600, 1920x1080; ctest 64/64. Not validated on a device.
- Next: long-press widget options, per-slot rotation, per-widget `compact` audit.

## 2026-10-13 (hourly run, 3)
- Merged #99 (area G map core, CI green). #100 (area H) still had the iPad check running; #73 was already stale.
- Long-press on a pane opens `PaneOptions` (change widget, next, previous, empty, reset all panes). ctest 71/71; screenshots rendered at 1024x600 and 800x480. ASan/UBSan left to CI (QML-only change). Not validated on a device.
- Next: per-slot rotation, per-widget `compact` audit.

## 2026-10-14 (hourly run)
- Merged #108 (area H, CI green). #107 (area G) still had Pi package builds running; #73 is stale with conflicts and not an agent/ui branch.
- Per-slot pane rotation: `AppSettings.paneRotations` / `setPaneRotation` (off, 5, 10, 30, 60 s; saved in options.ini, cleared by reset), a `PaneSlot` timer cycles through all widgets without writing settings, pauses while a menu is open; long-press menu has a "Rotate" button. ctest 80/80; screenshot 1024x600 unchanged. ASan/UBSan left to CI. Not validated on a device.
- Next: custom rotation sets per slot (category picker, A8), per-widget `compact` audit.

## 2026-10-14 (hourly run, 2)
- Issue #119: `Card` footer is now a status Text (elides, two lines in non-compact slots) plus a separate age Text that never elides, so the data age is always visible. ctest 82/82; screenshots 800x480 and 1024x600 checked (no fixtures for age, so only the status line shows). ASan/UBSan left to CI. Not validated on a device.
- #124 (QA docs) had CI still running at start; #73 stale. Next: custom rotation sets per slot (A8), per-widget `compact` audit.

## 2026-10-14 (hourly run, 3)
- Custom rotation sets per slot (A8): `AppSettings.paneRotationSets`, `togglePaneRotationMember`, `clearPaneRotationSet` (options.ini `panes/setN`, cleared by reset); `PaneSlot` rotates only through the set (empty = all widgets); long-press menu has Add/Remove from rotation and "Rotate all widgets". ctest 84/84; 1024x600 screenshot renders. ASan/UBSan left to CI. Not validated on a device.
- #132 (area G) had the iPad check still running; #73 stale. Per-widget `compact` audit is in src/widgets (other areas' files), so the mechanism (`Card.compact`) stays here and widget owners adopt it. Next: category picker, side panes.

## 2026-10-14 (hourly run, 4)
- Merged #143 (docs, CI green). #145/#146 still had macOS/iPad checks running; #73 stale.
- Category picker (A8): the pane chooser has tabs (All, Space Wx, DX & Contest, Weather, Sky & Space, Station, Other) that filter the widget list; opens on the current widget's category. Mapping lives in `PaneChooser.qml` (unknown ids fall under Other). ctest 88/88. The popup is not reachable from `--screenshot`, so its layout was not visually checked at 1024x600; only QML load/warnings are covered. Not validated on a device.
- Next: side panes / PANE_0 overlay, compact-mode adoption by widget owners.

## 2026-10-14 (hourly run, 5)
- Issue #150 (data never refreshes): added a timer-driven multi-cycle test for `PolledSource` (1 s interval, no `refreshNow`, >= 3 fetches). It passes on current code, so the shared timer/re-arm logic is not the cause; the earlier tests only used `refreshNow`. Remaining suspects are per-widget parsing (X-ray "missing" despite fresh data, area B) and the card age showing the slowest feed (per-value freshness, `Card` footer). ctest 94/94. Not validated on a device.
- #161/#162 (areas G/H) had checks still running; #73 stale. Next: per-value freshness in `Card`, #151 time zone picker, side panes.

## 2026-10-14 (hourly run, 6)
- Issue #151 (time zone picker): Settings > Station now uses `TimeZonePicker` (button + searchable popup: city/region filter, "City (UTC-7)" labels with the current offset, Up/Down/Enter/Escape, "Use this device's zone"); the system zone is preselected when none is saved. C++ side is `TimeZoneSearch` (`timeZones` context property) with `time-zone-search-regression`. ctest 105/105. Not done: suggesting the zone from DE/DX coordinates (needs a coordinate-to-zone table; with 0b location lookup). The popup is not reachable from `--screenshot`, so its layout is unchecked visually. ASan/UBSan left to CI. Not validated on a device.
- #170 (area H) had macOS/iPad checks running; #73 stale. Next: coordinate-based zone suggestion, per-value freshness in `Card`, side panes.

## 2026-10-14 (hourly run, 7)
- Issue #152 item 6: when DE == DX the station card shows "Set a DX target in Settings" instead of "0 KM · BEARING UNDEFINED" (`path_summary` in `src/ui/path_text.hpp`, `StationModel.pathText`, `path-text-regression`). The "NONAME" callsign was not found in src, so it is not addressed. ctest 111/111. ASan/UBSan left to CI. Not validated on a device.
- #179 (area H) had checks running; #73 stale. Remaining in #152: truncated-Text guard in the smoke test, wheel scrolling in Settings, centred Settings, satellite time format (G/other).

## 2026-10-14 (hourly run, 8)
- Issue #152: Settings page column is now centred on wide windows and scrolls with mouse wheel / trackpad (`WheelHandler` on the Flickable). ctest 115/115; QML loads without warnings. Settings is not reachable from `--screenshot`, so layout was not visually checked. ASan/UBSan left to CI. Not validated on a device.
- #186 had a failing macOS universal check (area F, not mine); #187 checks running; #73 stale.

## 2026-10-14 (hourly run, 9)
- Issue #152 automated guard: `--screenshot` now walks the item tree and exits 4 when a visible, non-empty `Text` reports `truncated` (items marked objectName `elideOk` are exempt; the `Card` status footer is, since long error reasons are shortened on purpose). New ctest `qml-app-truncation-800x480` / `-1920x1080` beside the existing 1024x600 smoke test, and the `qml-pane-*` tests only report it (`--allow-truncation`; CI fonts clip "Annular solar eclipse" in the eclipse widget, area owner to fix). It found a real clip: `Card` titles with the chooser arrow ("Space weather impact ▾") are now allowed to wrap to two lines. Settings is not reachable from `--screenshot`, so it is not covered. ctest 120/120. ASan/UBSan left to CI. Not validated on a device.
- #195 (area H) had the iPad check failing and macOS running; #73 stale. Not mine. Next: side panes / PANE_0, coordinate-based zone suggestion, per-value freshness in `Card`.
