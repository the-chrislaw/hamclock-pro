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

## 2026-10-14 (hourly run, 10)
- Backlog 0b slice 1 (offline): new `LocationTools` (`src/ui/location_tools.*`, QML `locationTools`): `parseCoordinate` (decimal, `40:44:55N`, `40°44'55" S`, hemisphere must match the axis), `fromGrid` (2/4/6 character square centre), `gridFor`, and `parseSearchReply` for Nominatim jsonv2 (tested on `tests/fixtures/sources/nominatim-search.json`). Settings > Station accepts DMS in the DE/DX fields and has a grid square field that fills the home coordinates. Test `location-tools-regression`; ctest 125/125. Not done: the "Find" button with the 1 request/s Nominatim call, zone suggestion, "Use my location". Settings is not reachable from `--screenshot`, so layout was not visually checked. ASan/UBSan left to CI. Not validated on a device.
- Only #73 was open (stale, not mine). Next: "Find" request slice, side panes / PANE_0, per-value freshness in `Card`.

## 2026-10-14 (hourly run, 11)
- Backlog 0c slice: tapping the station clock swaps the big clock between UTC and DE local time (`StationModel.deClock/deDate`, `clock_text.hpp`, `clock-text-regression`); the other zone shows as a small line under it. Screenshots at 800x480, 1024x600, 1920x1080 render with no truncation (the station is unconfigured in the fixtures, so the DE clock was not visible; layout of the configured card unchecked visually). ctest 128/128; ASan/UBSan left to CI. Not validated on a device.
- #212/#207 (other areas) had checks running or none; #73 stale. Next: "Find" request slice, DX local time/sun times, side panes.

## 2026-10-14 (hourly run, 12)
- Backlog 0c slice: the DE and DX boxes show the next sunrise and sunset as "Rise in 3h 12m · Set in 14h 5m" (soonest first; "Sun up/down all day" in polar regions), from the offline `astro_sun` code (`src/ui/sun_text.hpp`, `StationModel.deSun/dxSun`, `sun-text-regression`). Screenshots at 800x480, 1024x600, 1920x1080 with a configured station: the line wraps to two lines in the 1024x600 column, nothing clipped. Observed (not mine, not changed): `Main.qml` safe-area bindings log "Cannot read property 'left' of null" once at startup in `--screenshot`. ctest 130/130; ASan/UBSan left to CI. Not validated on a device.
- Only #73 open (stale, not mine). Next: "Find" request slice, DX local time/zone from coordinates, side panes.

## 2026-10-14 (hourly run, 13)
- Backlog 0c slice: tapping the path line in the station card toggles short path / long path ("Long path · 30023 KM · 270 DEG / TRUE"), using the existing offline `geometry()`; antipodal/coincident pairs keep the "Set a DX target" prompt (`path_detail` in `path_text.hpp`, `StationModel.toggleLongPath`, extended `path-text-regression`). The toggle is per session, not saved. Units (imperial) and magnetic bearing (WMM) still pending. ctest 132/132; configured-station layout not checked by screenshot this run. ASan/UBSan left to CI. Not validated on a device.
- #223 checks running, #220 no checks yet, #73 stale (none mine). Next: "Find" request slice, imperial units, side panes.

## 2026-10-14 (hourly run, 14)
- Backlog 0c/A12 slice: Settings > "Distance units" (Metric / Imperial) saved in `options.ini` (`units/imperial`, `AppSettings.imperialUnits/saveImperialUnits`); the station card path line shows "MI" instead of "KM" when imperial (`path_detail` gained an `imperial` argument; `path-text-regression` extended). The unconfigured/legacy distance string and magnetic bearing (WMM) are unchanged. Settings is not reachable from `--screenshot`, so the new card was not checked visually. Not validated on a device.
- #228 (area G) had the iPad check running; #73 stale. Not mine. Next: "Find" request slice, side panes.

## 2026-10-14 (hourly run, 15)
- Backlog A13 slice: the station card shows a callsign banner (configured stations only); tapping it cycles call / "ON AIR" / "73 DE call" (`banner_text.hpp`, `StationModel.bannerText/cycleBanner`, `banner-text-regression`). Per session only; fg/bg colours, rainbow and custom titles are pending. The fixture station is unconfigured, so the banner was not visible in 800x480, 1024x600, 1920x1080 screenshots (no regressions seen). ctest 133/133; ASan/UBSan left to CI. Not validated on a device.
- #236 (area G) checks running; #73 stale; neither mine. Next: "Find" request slice, side panes.

## 2026-10-14 (hourly run, 16)
- Backlog 0b slice: Settings > Station has a "Find a place" field and Find button. `LocationTools::search` makes one Nominatim request per press through the shared transport (honest User-Agent, max 1 request/s, min 3 characters, errors and "No match" shown) and fills latitude, longitude and grid. Offline test extended (`location-tools-regression`: injected transport, rate limit, failure). ctest 135/135. Not done: time zone suggestion from the result, "Use my location". Settings is not reachable from `--screenshot`, so layout was not checked visually. ASan/UBSan left to CI. Not validated on a device.
- #73 (conflicting, docs) and #242 (area F, no checks yet) open; neither mine, left alone.

## 2026-10-14 (hourly run, 17)
- Issue #151 remainder: `TimeZoneSearch::suggestZone(lat, lon)` picks the zone of the nearest city in a small offline table (about 110 cities, hint only, empty beyond 1500 km or for invalid coordinates); Settings > Station has a "Suggest zone from home coordinates" button that fills the picker and says to check it. Test `time-zone-search-regression` extended. ctest 137/137. DX zone from DX coordinates is not done. Settings is not reachable from `--screenshot`, so layout was not checked visually. ASan/UBSan left to CI. Not validated on a device.
- Only #73 was open (stale docs, not mine).

## 2026-10-14 (hourly run, 18)
- Backlog 0c/A14 slice: auxiliary time line under the station clock; tap cycles "Day N of 365 · week W", JD, MJD, local sidereal time (DE longitude, Greenwich when unconfigured), UNIX (`aux_time.hpp`, `StationModel.auxTime/cycleAuxTime`, `aux-time-text-regression` checked against J2000 values). Per session only; date order/week start, time travel and alarms pending. ctest 138/138; 800x480, 1024x600, 1920x1080 screenshots show no clipping. ASan/UBSan left to CI. Not validated on a device.
- Only #73 open (conflicting docs, not mine). Pre-existing warnings in legacy src/qt dialogs remain (retire task).

## 2026-10-14 (hourly run, 19)
- Issue #261: the station card title is now "Station" while the banner shows the plain callsign, so the callsign appears once (the title stays as the chooser tap target); ON AIR / 73 modes show the callsign as the title. New `banner_is_callsign()` (`banner-text-regression` extended) and `StationModel.bannerIsCallsign`. Screenshot at 1024x600 with `callsign=W1TEST`: one callsign, no clipping; 800x480 rendered too. ctest 138/138; ASan/UBSan left to CI. Not validated on a device.
- #271 (docs, not mine) had macOS checks running; #73 stale. Left alone.

## 2026-10-14 (hourly run, 20)
- Backlog 0/A8: the pane chooser already had category tabs, but 11 widgets (ADS-B, APRS, planets, eclipse, EME, daylight, GPS, radio, rotator, Dst, space weather impact) fell under "Other". They now sit in their categories, and the new `pane-categories-regression` (python, offline) fails if any widget registered in `src/widgets` has no category. One line added to the root CMakeLists test block. ctest 139/139; no layout change, so no new screenshots. ASan/UBSan left to CI. Not validated on a device.
- #73 (docs, no checks), #274 (tvOS check queued) and #279 (no checks yet) were open; none mine and none mergeable yet, left alone.

## 2026-10-14 (hourly run, 21)
- Backlog A14 slice: Settings > "Clock format" chooses the date order (Thu 15 Oct / Thu Oct 15 / 2026-10-15) and 12/24 h clock, saved in `options.ini` (`clock/date_order`, `clock/h12`; `AppSettings.saveClockFormat`); applies to the UTC and DE clocks in the station card (`clock_text.hpp`, `clock-text-regression` extended). Screenshots at 800x480, 1024x600, 1920x1080 with 12 h + ISO: no clipping. Week start, alarms and time travel still pending. ctest 140/140; ASan/UBSan left to CI. Not validated on a device.
- #279 and #286 (others) still had checks running; #73 stale. Left alone.

## 2026-10-14 (hourly run, 22)
- Backlog 0c slice: the DX box shows "Local 06:08 · Tokyo (auto)", the DX local time using the offline nearest-city zone hint (`zone_local_text` in `clock_text.hpp`, `StationModel.dxLocal`, `clock-text-regression` extended). Hidden when no zone is within 1500 km. Honest limit: borders are not modelled, so the zone can be wrong near them; manual override still pending. Screenshots at 800x480 and 1024x600 with a configured station (DX Tokyo): fits in two lines at worst, no clipping; 1920x1080 rendered. ctest 142/142; ASan/UBSan left to CI. Not validated on a device.
- #73 (docs, conflicting) and #293 (area G) open; not mine, left alone.

## 2026-10-14 (hourly run, 23)
- Backlog A14 slice: Settings > "Clock format" has a week start choice (Mon = ISO 8601, Sun = US, week 1 contains 1 January), saved in `options.ini` (`clock/week_start`, `AppSettings.saveWeekStart`); it sets the week number in the auxiliary time line (`week_of_year` in `aux_time.hpp`, `aux-time-text-regression` extended). Calendar widgets are not wired to it. Settings is not reachable from `--screenshot`, so layout was not checked visually. ctest 142/142; ASan/UBSan left to CI. Not validated on a device.
- #300/#301 (others) still had macOS checks running; #73 stale. Left alone.

## 2026-10-14 (hourly run, 24)
- Backlog A2 slice: Settings > "TV overscan" (Off, 2-10 %) keeps a margin of that share of width/height free on every edge, saved in `options.ini` (`display/overscan`, `AppSettings.saveOverscan`, validated 0..10; `widget-core-regression` extended). Added to the safe-area margins in `Main.qml`. Screenshot at 1024x600 with 6 % shows the inset and no clipping; 1920x1080 rendered. Settings page layout not viewable via `--screenshot`. Focus order and focus ring still pending. ctest 144/144; ASan/UBSan left to CI. Not validated on a device.
- Merged #308 (green). #309 checks running; #73 stale. Left alone.

## 2026-10-15 (hourly run, 25)
- Backlog 0c/A5 slice: stopwatch and countdown as a QML page (`TimersPage.qml`), opened by long-pressing the station clock. Logic is `timers.hpp` (injected time, `timers-regression`: start/stop/reset, countdown pause/expire, formatting); `TimersModel` drives it at 100 ms while running. Session only; no alarm sound, Big Clock and the Qt Widgets dialogs are not retired yet. The page is not reachable from `--screenshot`, so its layout was not checked visually (QML loads without warnings). ctest 146/146; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 26)
- Backlog A7 slice: `src/ui/kiosk/hamclock-pro-kiosk.service` (eglfs, `--fullscreen`, non-root user, NoNewPrivileges, ProtectSystem=strict) and `KIOSK.md` (user, groups, eglfs/linuxfb fallback, touch calibration, overscan, logs). New `kiosk-unit-regression` (python, offline) checks the unit's key lines. Not installed by CMake yet and not run on a Pi or under systemd; on-screen keyboard decision still open. No UI change, so no screenshots. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 27)
- Backlog A14 slice: daily and one-time alarms in UTC (`alarms.hpp` pure logic with injected time: next-ring, window-based firing with no double ring, clock jumps over 5 minutes ignored; `AlarmsModel`; `alarms-regression`). The Timers page has a time stepper (15 min) with Daily/Once buttons and a tap-to-delete list; a ringing banner appears over the dashboard until dismissed. Session only, max 8 alarms, visual only (no sound, no GPIO), not persisted. The Timers page is not reachable from `--screenshot`, so its layout was not checked visually; the dashboard still loads with no QML warnings. ctest 148/148; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 28)
- Backlog A2 slice: keyboard / TV remote on the focused pane: Left/Right step the widget, the Menu key opens the pane options (`PaneSlot.qml` key handlers; Tab focus ring already existed). `pane-keys-regression` (python, source-level: no QML key-event harness exists yet). Arrow-key focus moves between panes still pending. No layout change, so no new screenshots. ctest 149/149; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.
