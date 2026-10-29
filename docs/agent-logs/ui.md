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

## 2026-10-15 (hourly run, 29)
- Backlog A2 slice: Up/Down on a focused pane move the focus ring to the previous/next focusable item (Tab order) via `PaneSlot.moveFocus`; `pane-keys-regression` extended (source-level). No layout change, so no new screenshots. ctest 151/151; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 30)
- Backlog A13 slice: Settings > "Callsign banner" colour (Theme, Amber, Green, Red, White, Rainbow), saved in `options.ini` (`banner/style`, `AppSettings.saveBannerStyle`); `banner_color` in `banner_text.hpp` (rainbow turns hue once per 12 s from the clock), `banner-text-regression` extended. Text size is unchanged, so layout is the same; 800x480 and 1024x600 render with exit 0 and no truncation. Custom title and background colour still pending. ctest 151/151; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 31)
- Backlog A13 slice: Settings > "Callsign banner" has a custom title (max 24 characters, `banner/title` in `options.ini`, `AppSettings.saveBannerTitle`); when set, tapping the banner gets a fourth step showing it (`banner_text.hpp`, `banner-text-regression` and `widget-core-regression` extended). Background colour still pending. Settings is not reachable from `--screenshot`, so its layout was not checked visually; dashboard renders at 800x480, 1024x600, 1920x1080 with exit 0.
- Fixed a latent crash: `AppSettings::station_` was default-initialised, so with no station file the DE/DX coordinates were indeterminate and `BandsModel` randomly threw "invalid location" (QML smoke tests aborted ~60 % of runs once the class layout changed). Now value-initialised (zeros). ctest 151/151, repeated smoke runs 0/15 failures; ASan/UBSan subset (banner, widget-core, smoke) clean, full sanitizer run left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 32)
- Backlog A13 slice: Settings > "Callsign banner" has a background fill (No fill, Black, Navy, Maroon, White; `banner/background` in `options.ini`, `AppSettings.saveBannerBackground`, `banner_background` in `banner_text.hpp`); the banner draws on a rounded rectangle of that colour. Tests: `banner-text-regression`, `widget-core-regression` extended (validation, persistence). Screenshots at 800x480, 1024x600 (navy fill, amber text, no clipping), 1920x1080 exit 0. The Settings page is not reachable from `--screenshot`, so its layout was not checked visually. ctest 151/151; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 33)
- Backlog A0c slice: manual DX time zone override. Settings > Station has a "DX time zone" picker with Save and Auto (`dx/zone` in `options.ini`, `AppSettings.saveDxZone`, validated with `QTimeZone`); `StationModel.dxLocal` uses it, else the automatic zone from the DX coordinates. `widget-core-regression` extended (reject, persist, clear). Settings is not reachable from `--screenshot`, so layout was not checked visually. ctest 151/151, no warnings; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 34)
- Backlog A15 slice: software screen blanking. `blanking.hpp` (pure: idle timeout 0..240 min, daily local night window that may cross midnight, a touch wakes the screen for 60 s inside the window), `BlankingModel`, Settings > "Screen blanking" (`kiosk/blank_*` in `options.ini`, `AppSettings.saveBlanking`), black overlay in `Main.qml` that any tap dismisses; a passive `TapHandler` counts touches as activity. Overlay only: backlight/DPMS are not touched. Tests: `blanking-regression` (new), `widget-core-regression` extended (validation, persistence). Dashboard renders at 800x480, 1024x600, 1920x1080 with no QML warnings; Settings is not reachable from `--screenshot`, so its layout and the overlay were not checked visually. Keyboard activity does not wake yet. ctest 152/152; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 35)
- Backlog A15 slice: keyboard activity now wakes the blanked screen and counts as activity (`PaneSlot.qml` passive `Keys.onPressed`; the blank overlay takes focus and any key wakes it). `pane-keys-regression` extended (source-level). No layout change, so no new screenshots. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 36)
- Backlog A0d slice: portrait phone layout (width < 600 and taller than wide): `Theme.phone`, page grid becomes one column (station strip 36 % with DE and DX side by side, one pane 22 %, map fills the rest); other sizes unchanged (1024x600 and 800x480 re-rendered, identical). New truncation test `qml-app-truncation-390x844`. Rendered at 390x844 with a configured station: no clipping. Landscape phone (844x390) still uses the desktop grid; page dots pending. ctest 153/153; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs) and #383 (Apple, conflicting) were open; left alone. Issue #151 (time zone picker) is already implemented.

## 2026-10-15 (hourly run, 37)
- Backlog A0d slice: landscape phone (844x390) rendered with the desktop grid is clean (station column, 3 panes, map, no clipping), so no layout change; added the truncation guard `qml-app-truncation-844x390`. Page dots and a dedicated landscape layout remain pending. ctest truncation set 4/4; full run and ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 38)
- Backlog A0d slice: page dots on the portrait phone pane (window of up to 7 dots around the current widget, passive so taps and swipes still work; `PaneSlot.qml`). `pane-keys-regression` extended (source-level). Rendered at 390x844 (dots sit at the pane bottom, no clipping) and 1024x600 with exit 0 and no QML warnings; other sizes unchanged. ctest 154/154; ASan/UBSan left to CI. Dedicated landscape phone layout still pending. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 39)
- Backlog A0d slice: dedicated landscape phone layout (height < 440 and wider than tall): station column, map left, one pane on the right (30 % width), `Main.qml` `phoneWide`; the page-grid inner column became a right-to-left `GridLayout`. 800x480, 1024x600 and portrait unchanged. Rendered 844x390: no clipping. A0d now complete. Not validated on a device.

## 2026-10-15 (hourly run, 40)
- Issue #401 (station path line wrapped with an orphan "·" / "DEG / TRUE"): `path_detail` now gives three deliberate lines ("Short path", "15991 KM", "266° TRUE") with no-break spaces inside value and unit; `path-text-regression` extended (line count, no breakable space). Rendered with a configured station at 800x480, 1024x600, 1920x1080: no clipping or orphans. ctest 154/154; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 41)
- Backlog A0c/A12 slice: tapping the sunrise/sunset line in the DE or DX box toggles "Rise in 3h 12m" and UTC clock times ("Rise 06:02z"); per session (`StationModel.toggleSunAt`, `sun_line(..., at)`). `sun-text-regression` extended. Dashboard renders at 1024x600 and 800x480 with no QML warnings; the default station is unconfigured in `--screenshot`, so the sun line itself was not seen visually. ctest 154/154; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 42)
- Backlog A14 slice: alarms persist. `alarms_serialize/alarms_parse` (pure, "HHMM,d|o,1|0", bad entries dropped, max 8) and `AlarmsModel(optionsPath)` saves `alarms/list` in `options.ini` on add, remove, enable and a one-time alarm disabling itself. `alarms-regression` extended (round trip, rejection, model reload with a temp dir). No layout change, no new screenshots. Full ctest passed; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 43)
- Backlog A16 slice: saved configurations. `config_store.hpp` (pure: names 1-24 chars, max 12, copies `options.ini` to `configs/<name>.ini`, atomic restore, rename, delete; station and credentials never included), `AppSettings` invokables and a Settings > "Saved configurations" card. Restore reloads AppSettings at once; pane slots, alarms and blanking need a restart (said in the UI). Test `config-store-regression` (new). A13 note corrected (banner background was already done). Settings is not reachable from `--screenshot`, so the card was not checked visually; ctest 157/157, no new warnings; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-15 (hourly run, 44)
- Backlog A5 slice: retired the standalone `hamclock-pro-editor` (target, `station_editor.cpp`, install entry, README row; Settings > Station in the Qt Quick app covers it). `station_editor_dialog` stays: the legacy dashboard and its tests still use it. Dashboard and map-preview remain until the map card replaces them. Root `CMakeLists.txt` touched only to drop that target (minimal, explained in the PR). Build clean of new warnings, ctest 147/147; ASan/UBSan left to CI; not validated on a device.

## 2026-10-15 (hourly run, 45)
- Backlog A0c/A5 slice: Big Clock page (`BigClockPage.qml`): UTC and local time in the largest type that fits, dates, zone and the auxiliary time line; opened by a "Big Clock" button on the Timers page (long-press the clock), tap anywhere or Escape closes. `pane-keys-regression` extended (source-level wiring check). The page is not reachable from `--screenshot`, so its layout was not checked visually; the dashboard still renders at 1024x600 with no QML warnings. ctest 157/157; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone. Note: local `origin/main` ref was stale at start; fetched before branching.

## 2026-10-16 (hourly run, 46)
- Merged #417 (Big Clock); its macOS universal job failed on a runner "hdiutil: Resource busy" DMG error, not code, and the Qt checks were green.
- Backlog A0b slice: Settings station card now suggests the time zone right after Find or grid entry (shared `suggestZone`; the button remains). Settings is not reachable from `--screenshot`, so not checked visually; build/ctest results in the PR. Not validated on a device.

## 2026-10-16 (hourly run, 47)
- Backlog A5 slice: retired the standalone `hamclock-pro-map-preview` (target, `src/qt/map_preview.cpp`, `docs/qt-map-preview.md`, install entry, README row); the QML map card covers it. Root `CMakeLists.txt` touched only to drop that target (minimal, explained in the PR). `hamclock-pro-dashboard` remains. Pre-existing legacy warnings (de_clock dialogs, one test) are unchanged. ASan/UBSan left to CI. Not validated on a device.

## 2026-10-16 (hourly run, 48)
- Backlog A0 slice (every pixel on 7-inch): `Card.showTitle`; the map card hides its "World map" title row on dense screens (the in-map "Map" button stays), giving the map about 30 px more height at 1024x600. Rendered 1024x600, 800x480, 1920x1080 light/dark: exit 0, no clipping, no QML warnings. ctest 157/157; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-16 (hourly run, 49)
- Backlog A14 slice: auxiliary time line gains a "Solar" mode (apparent solar time = UTC + longitude + equation of time, about a minute accurate; uses the DE longitude), last in the tap cycle. `aux-time-text-regression` extended (value range, longitude shift, wrap, cycle). Text-only change; no layout change, no new screenshots. ctest 157/157; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-16 (hourly run, 50)
- Backlog A15 slice: Settings > "Exit" card (hidden on iOS): "Exit HamClock Pro" needs a second tap within 5 s, then `Qt.quit()`. Deliberately no reboot/shutdown (privileged). `pane-keys-regression` checks the two-tap wiring. Settings is not reachable from `--screenshot`, so not checked visually. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-16 (hourly run, 51)
- Backlog A5 slice: retired the last standalone Qt Widgets program, `hamclock-pro-dashboard` (target, `dashboard_main.cpp`, `station_dashboard.*`, `qt-dashboard-regression`, install entry, README row; README now lists `hamclock-pro`). Root `CMakeLists.txt` touched only to drop that target (minimal, explained in the PR). The legacy dialogs and `world_map_widget` stay for their own tests. Full build with no warnings, ctest 156/156; no UI change so no screenshots beyond a 1024x600/800x480 sanity render. ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs) and #443 (apple, conflicting) were open; left alone.

## 2026-10-16 (hourly run, 52)
- Backlog A0 slice (every pixel on 7-inch): unconfigured station card hints shortened ("Set a time zone in Settings", "Add callsign and location to see paths and local time.") so they take fewer wrapped lines at 1024x600. Rendered 1024x600, 800x480, 1920x1080: exit 0, no QML warnings, no truncation. ctest 156/156; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-16 (hourly run, 53)
- Backlog A2 slice (TV large type): Settings > "Text size" (100/115/130/150 %, `display/text_scale` in `options.ini`) multiplies `Theme.scale`. `widget-core-regression` extended (default, rejection, apply, restart). Rendered 1024x600 and 1920x1080 with 130 % and the default: exit 0, no QML warnings; I did not inspect the images (Settings itself is not reachable from `--screenshot`). ctest 156/156; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-16 (hourly run, 54)
- QA #454 (text size 150 % truncated the pane chooser title at 800x480): the card header's status badge now drops to a dot (word kept in the accessible name) when the title plus the full badge would not fit, so the title keeps the width and stays whole. Rendered 800x480 at 150 %: no TRUNCATED report, titles whole (two lines where needed). The footer status reason is still shortened on purpose (`elideOk`); the data age is never cut. 1024x600 at 150 % also exit 0. ctest 156/156; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-16 (hourly run, 55)
- Backlog A8 guard: `pane-keys-regression` now fails when a widget registered with `registry.add({"id", ...` has no category in `PaneChooser.qml` (it would silently fall under "Other"). Checked it passes today (41 widgets, all covered) and fails when a category entry is removed. Test-only change; no UI change, no screenshots. Not validated on a device.
- Only #460 (QA log, still in CI) and #73 (stale docs, not mine) were open; left alone.

## 2026-10-17 (hourly run, 56)
- Backlog A17 slice: Settings > "Accent colour" (blue/green/amber/red/purple, `display/accent` in `options.ini`) drives `Theme.accent` with separate light/dark values. `widget-core-regression` extended (default, rejection, apply, restart). Rendered 1024x600 and 800x480 light/dark with amber: exit 0, no truncation report; I did not inspect the images (Settings not reachable from `--screenshot`). ctest 156/156; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-17 (hourly run, 57)
- Backlog A15 slice: the night blanking window gets per-weekday toggles (Mon..Sun, `kiosk/blank_days` bitmask, default all days; a window crossing midnight belongs to the day it starts). `blanking_regression` and `widget-core-regression` extended (weekday edges, mask validation, save/restart). Settings not reachable from `--screenshot`, so not checked visually; 1024x600 sanity render ran. ctest 158/158; ASan/UBSan left to CI. Pre-existing legacy warnings unchanged. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-17 (hourly run, 58)
- Backlog A16 slice: restoring a saved configuration now re-applies pane slots at once (`panesChanged` emitted; blanking already followed `optionsChanged`); only alarms still need a restart. `widget-core-regression` extended. Settings text updated; no layout change, not reachable from `--screenshot`. ctest 160/160; ASan/UBSan left to CI. Pre-existing legacy warnings unchanged. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-17 (hourly run, 59)
- Layout audit only, no code change: rendered 1024x600 and 800x480 (dark) with `--fixtures tests/fixtures/sources`: exit 0, no QML warnings, no truncation report, no header bar, map fills its card. With that fixture directory the data panes show their offline/unconfigured states ("No fixture for /..."), so live-data pane density was not checked visually this run; the fixture file names do not match what the widgets request. Light theme and 1920x1080 not rendered. No backlog slice started (the remaining A items need hardware, side panes, or per-widget compact layouts owned by other areas). Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-18 (hourly run, 60)
- Layout audit on current main with real data: merged every `src/widgets/*/tests/fixtures` file into one directory (the fixture server matches on the URL's file name) and rendered 1024x600 and 800x480 (light, plus an earlier dark pass): exit 0, no truncation report, no header bar, map fills its card, station column and three panes fit without scrolling. The default DX cluster pane shows only "Set up" until a host is saved, so that slot is mostly empty on a fresh install (content is area C's; a compact "not configured" layout is the fix). 1920x1080 not rendered. No code change; remaining A items need hardware, side panes, or per-widget compact layouts owned by other areas. Not validated on a device.
- Only #73 (stale, conflicting docs PR, not mine) was open; left alone. Open `[A]` qa issues #150-152 are addressed in earlier entries.

## 2026-10-18 (hourly run, 61)
- Backlog A14 slice (Big Clock option parity): the Big Clock page shows local sidereal time at DE beside the auxiliary line (`StationModel::lstTime`, same `aux_time_text` maths already covered by `aux-time-text-regression`; not repeated when the aux line is already LST). Full build, ctest 160/160, pre-existing legacy warnings unchanged. The Big Clock page is not reachable from `--screenshot`, so not checked visually; ASan/UBSan left to CI. Not validated on a device.
- Open: #500 (QA log, still in CI, not mine) and #73 (stale docs, not mine); left alone.

## 2026-10-19 (hourly run, 62)
- Backlog A16 slice (live alarm re-apply): restoring a saved configuration now reloads the alarm list at once (`AlarmsModel::reload()` on `optionsChanged`); Settings text updated. `alarms-regression` extended (file changed under a live model). ctest 163/163; ASan/UBSan left to CI. No layout change, so no screenshots. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-20 (hourly run, 63)
- Backlog A16 slice (demo mode): Settings > "Demo mode" starts a session-only mode (`AppSettings::demoMode`, never saved) in which every pane steps through all widgets every 10 s with real data (no invented data); "Stop demo" is in the pane long-press menu. `widget-core-regression` extended (off by default, saved rotation untouched). 1024x600 render ran with exit 0; Settings and the long-press menu are not reachable from `--screenshot`, so not checked visually. ctest 164/164; ASan/UBSan left to CI. Pre-existing legacy warnings unchanged. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-20 (hourly run, 64)
- QA #532: the truncation guard (`count_truncated` in `src/ui/main.cpp`) now also reports Text whose painted extent (contentWidth x contentHeight, alignment-aware) runs outside its nearest clipping ancestor or the window, unless `objectName` is `clipOk`/`elideOk` or it sits under a Flickable/ListView. Proved the detector fires (temporary negative tolerance listed 'Your station', clock etc.), then ran all 42 registered panes (x3 in the pane slots) at 800x480 and 1024x600 dark with merged widget fixtures: no clipped text reported here. The Pi-only clipping in the issue (Sun & Moon DX column, Host sensors clock) depends on Pi fonts that this build machine does not have, so it is not reproduced here. `qml-app-truncation-*` now also runs at 1024x600. ctest qml-* 48/48; full ctest and ASan/UBSan left to CI. Not validated on a device.

## 2026-10-21 (hourly run, 65)
- QA #527: README rewritten for users (what it is, independence notice, four `--screenshot` renders under `docs/images/` at 1024x600 and 1920x1080 light/dark from recorded sample data, pane features grouped like the chooser, install options stated honestly as planned/unvalidated, sources, MIT, bug reporting); developer build/test/CLI notes moved to `docs/DEVELOPING.md`. No macOS window screenshot (no mac here). Docs and images only; not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-21 (hourly run, 66)
- Issue #526 first slice (owner exception: new `.github/workflows/image.yml` and `packaging/pi/`): `--first-run` opens Settings at start when no station is configured (rendered 1024x600 dark: Settings shown first, exit 0); pi-gen config + `stage-hamclock` (installs the release .deb, cage kiosk unit, locked password, SSH off, hourly `hamclock-pro-update` timer using an honest User-Agent against GitHub Releases); `docs/install-pi.md`. The workflow has never run and the image has not been built, booted in QEMU or flashed; remaining: a CI build/size check, QEMU boot, Settings first-run wording for Wi-Fi. ctest 167/167; ASan/UBSan left to CI. Not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-21 (hourly run, 67)
- QA #546: the Pi image updater is now opt-in (timer no longer enabled in the image), uses `releases/latest` (stable only, never the rolling pre-release), matches only asset `browser_download_url`s, requires a matching `SHA256SUMS` release asset (fails closed), keeps the previous `.deb` and reinstalls it if the install fails. Docs updated. Not done: detached-signature verification (needs an owner-held signing key), publishing `SHA256SUMS` from the release workflow (workflows not mine), checksum on the base `.deb` in `image.yml`. Script checked with `sh -n` only; not run, image not built, not validated on a device.
- Only #73 (stale docs, not mine) was open; left alone.

## 2026-10-23 (hourly run, 68)
- QA #574 audit (no code change): rendered dark with merged widget fixtures at 390x844, 844x390, 430x932 and 932x430: exit 0, no QML warnings, no truncation report; portrait stacks station strip, one pane (page dots) and map, landscape puts the map left and one pane right. With no station configured the portrait station card is mostly empty (Set up button only). Not done: the Passes / Find-in-the-sky entry points (no such widgets exist yet; areas D and F), Settings one-handed pass, App Store-size renders (6.9"/13" need exact pixel sizes). Not validated on a device.
- Open: #577 and #578 (other areas, CI still running at the time) and stale #73; left alone.

## 2026-10-24 (hourly run, 69)
- QA #587 (800x480 crop): root cause was the `--screenshot` tool, not the layout. `window->resize()` reaches the content item asynchronously on the offscreen platform, so the grab showed the initial 1024x600 layout cropped to 800x480 (confirmed: window 800x480, contentItem 1024x600). Now the tool pumps events until the content item matches the requested size and exits 6 if it still does not. New `qml-app-station-800x480/1024x600` tests run with a configured station (`tests/fixtures/station-configured.conf`). Rendered dark/light at 800x480, 1024x600, 1920x1080: exit 0, no truncation, all three panes, map controls and Settings button visible at 800x480. ASan/UBSan: the new configured-station tests exposed that telnet-based panes (APRS-IS, DX cluster, ...) opened real sockets under `--offline`/`--fixtures`; `set_telnet_network_enabled(false)` (src/net, shared, backward compatible) now makes those sockets fail like a dead network in those modes. ctest 175/175 in both the normal and ASan/UBSan builds. The test cannot reproduce the old crop without the fix (race-dependent), so the exit-6 guard is the regression check. Not validated on a device.
- Open: #73 (stale, not mine); left alone.

## 2026-10-24 (hourly run, 70)
- Backlog A14 slice (Big Clock face): Settings > "Big Clock face" Digital/Analog (`display/bigClockFace`, saved); analog draws a UTC dial (ticks, hour/minute/second hands, accent second hand) above the digital lines. `widget-core-regression` extended (default, bad value rejected, applied, survives restart). ctest 176/176. Rendered the page temporarily at 800x480 and 1024x600 dark (exit 0, hands match the digital time; 1920x1080 not rendered); the page is still not reachable from `--screenshot`. ASan/UBSan left to CI. Not validated on a device.
- Open: #73 (stale, not mine); left alone. `[A]` qa issues already addressed in earlier entries.

## 2026-10-25 (hourly run, 71)
- No code change. No open PR of mine; the open `[A]` qa issues (#527, #532, #546, #526, #574) are covered by earlier entries. What remains in area A needs a device (kiosk validation, brightness/backlight, "Use my location") or touches map code owned by area G (A17 path/band colours). Nothing invented.
- Open: #73 (stale docs, not mine); left alone.

## 2026-10-26 (hourly run, 72)
- No code change. No open PR of mine; the open `[A]` qa issues (#527, #532, #546, #526, #574) are covered by earlier entries. Remaining area-A work needs a device or belongs to area G. Nothing invented.
- Open: #73 (stale docs, conflicting, not mine); left alone.

## 2026-10-29 (hourly run, 73)
- QA #638 (blank pane grid at 1920x1080): root cause was the inner `GridLayout` in `Main.qml`: when the fitted column count changed while the layout settled (6 -> 5), every slot stayed at 0x0 at (0,0) although the row itself was sized correctly (measured with a temporary slot-geometry log). `paneRow` is now a plain `Item` with explicit slot x/y/width/height from `grid.columns/rows`. Regression: PaneSlot has `objectName: paneSlot` and the screenshot guard (exit 4) reports visible slots with no size (it fired 5x on the old layout); `qml-app-station` now also runs at 1920x1080 and 2560x1440. Sun & Moon "Next season" (area B file, one-number change, minimumPixelSize 0.85 -> 0.7 like its neighbours) was truncated at 1920x1080 once the panes drew. Rendered dark 800x480, 1024x600, 1280x720, 1920x1080: all panes visible, exit 0. Not validated on a device.

## 2026-10-29 (hourly run, 74)
- QA #652 (Sun & Moon header badge clipped at 1512x982): root cause was `StatusBadge.implicitWidth` reading `row.implicitWidth`; when `dotOnly` flipped back to false while the pane widened (137 -> 298 px), the header RowLayout kept the title sized for the 19 px dot and never re-laid out, so the 62 px badge ran past the card (title 273 + badge 62 > 298). The width now comes straight from `dotOnly`/`fullWidth` (same values), so the layout tracks it. Measured with temporary logging: badge x 237 + w 61 = 298 = row width. `qml-app-truncation-1512x982` failed before (reproduced locally, exit 4) and passes now; full ctest 187/187. Rendered light/dark at 800x480, 1024x600, 1512x982, 1920x1080: exit 0, no truncation report. The CI font set likely settles the layout differently, which is why CI was green before; the fix removes the dependence on that. ASan/UBSan left to CI. Not validated on a device.
