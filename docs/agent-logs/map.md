# Agent log: map

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (UTC)

- Slice 1 of G1 (PR agent/map-purepainter): new QtGui-only `MapRenderer` (`src/map/map_renderer.*`) and pure view maths `MapViewport` (`src/map/viewport.*`). `MapItem` now uses it, so the QML map has no QtWidgets dependency. The map fills its item (issue #79): scale covers the item, polar rows are cropped but never below ±70° (very wide panes wrap longitude instead), tall panes crop longitude around DE, and DE/DX latitudes shift the crop to stay visible. Main.qml: the map now uses `anchors.fill: parent` instead of a forced 2:1 box (shared file, minimal). Land, coast, night shade, subsolar point and DE/DX path are unchanged; tests: new `qt-map-viewport-regression`.
- Screenshots checked offscreen at 800x480, 1024x600, 1920x1080 (light/dark). Not validated on device.
- Not merged by me: PR #73 (docs, not mine) has merge conflicts; left alone.
- Next: switch the legacy `WorldMapWidget` onto `MapRenderer` (then drop QtWidgets from the map library), then G2 projections (Mercator centre longitude, azimuthal, Robinson), zoom, tap Set DE/DX.

## 2026-10-13 (UTC, run 2)

- G1 slice 2 (PR agent/map-widget-renderer): split a QtGui-only `hamclock-map-core` static library (renderer, viewport, projection, path, land geometry, solar maths) from `hamclock-world-map`, which now holds only the legacy QWidget map and links the core. The Qt Quick app (`hamclock-pro`, `hamclock-widget-core`) links the core only and runs on `QGuiApplication`, so the main app no longer depends on QtWidgets (shared files touched minimally: `cmake/HamClockApp.cmake`, `src/ui/main.cpp`). Legacy dashboard/preview/editor tools keep QtWidgets until retired; their tests are unchanged and pass (71/71 offscreen). 1024x600 screenshot checked: map fills its card.
- Merged #97 (docs, green). #73 (not mine) still conflicts; left alone. #96/#98 still had CI running.
- Not validated on device.
- Next: G2 overlay API (`src/map/overlay.*`, `docs/map-overlays.md`) so areas B/C/D/E/H can start map items.

## 2026-10-13 (UTC, run 3)

- Merged #105 (wx sensors, green, auto-merge). #73 (docs, not mine) still conflicts; left alone.
- G2 overlay API (PR agent/map-overlay-api): `src/map/overlay.*` (markers, layers, labels None/Prefix/Call/Dot, minimum label distance, optional DE path, process-wide `MapOverlays` registry), drawn by `MapRenderer` above DE/DX; `MapItem` listens and repaints (shared file touched minimally: `CMakeLists.txt` library/test lists). Docs and worked example in `docs/map-overlays.md`. Test: `qt-map-overlay-regression`.
- Not validated on device.
- Next: G3 map styles framework; then G4 projections.

## 2026-10-14 (UTC)

- Orient: no `qa` issues open. #114 (not mine) still had the iPad job running; #73 (docs, not mine) stale, left alone.
- G3 map styles framework (PR agent/map-styles): `src/map/styles.*` (`MapStyle`, `ColorScale`/`color_at`, process-wide `MapStyles` registry, rotation 20/60/90/120 s, automatic space-weather pick aurora/drap), drawn by `MapRenderer` between coast and night shade plus a gradient legend; `MapItem` follows the registry (shared: `CMakeLists.txt` lists, refresh timer 30 s -> 10 s so rotation steps). Test: `qt-map-styles-regression`; ASan/UBSan run of the map tests clean. Screenshot at 1024x600 unchanged with no style registered. Not validated on device.
- Next: G4 projections (Mercator centre longitude, azimuthal, Robinson), zoom, tap Set DE/DX.

## 2026-10-14 (UTC, run 2)

- Orient: #122 ([G] qa) first. #130 (not mine) had macOS still running; #73 (docs, not mine) conflicts; left alone.
- Label declutter (PR agent/map-label-declutter, fixes #122): DE/DX labels and markers are laid out first and act as obstacles; overlay labels go through new `place_label` (right of the marker, flipped left at the map edge, clamped vertically, skipped when overlapping any used rect). `MapScene::ui_scale` / `MapItem.uiScale` (bound to `Theme.scale` in Main.qml, shared file, one line) scales label font and marker radii. Test: extended `qt-map-overlay-regression`. Offscreen 1024x600 and 1920x1080 checked: labels readable, KH6RS flips at the edge. Not validated on device.
- Next: G4 projections (Mercator centre longitude, azimuthal, Robinson), zoom, tap Set DE/DX.

## 2026-10-14 (UTC, run 3)

- Orient: no `[G]` qa issues (#120 is B). #139 (not mine) still had a macOS job running; #73 (docs, not mine) stale; left alone.
- G4 slice a (PR agent/map-zoom-tap): `MapViewport::fill` takes zoom (1-4x), pan lon/lat; `location_at` inverts a pixel to lat/lon. `MapItem`: `zoom`, `resetView()`, `panBy()`, drag-to-pan, and `tapped(lat, lon)` for a tap without drag. Main.qml: small +/-/reset buttons on the map card (shared, minimal). Test: extended `qt-map-viewport-regression`. 1024x600 dark screenshot checked offscreen. Not validated on device.
- Next: G4 slice b: Set DE / Set DX popup on `tapped` (needs AppSettings station write, area A coordination), then Mercator centre longitude, azimuthal, Robinson.

## 2026-10-14 (UTC, run 4)

- Orient: #141 ([G] qa) first. #143 (docs, not mine) and #73 (docs, conflicts) left alone.
- Fix for #141 (PR agent/map-shared-projection): `MapRenderer` set the land/coast/night transform with `setTransform(t, false)`, which replaced the painter's base transform. Under `QQuickPaintedItem` with a contents/device scale (Retina) the land lost that scale while markers, DE/DX and the sun (plain coordinates) kept it, so overlays and land used different rectangles. Now combined (`true`); one viewport and one base transform for everything. Test: `qt-map-overlay-regression` paints at 1024x600 and 1024x630 under `scale(2)` and compares with a direct 2x render (fails without the fix, passes with it). Not reproduced on macOS hardware; not validated on device.
- Next: G4 slice b: Set DE / Set DX popup on `tapped`, owner's map controls priority (#143), then Mercator centre longitude, azimuthal, Robinson.

## 2026-10-14 (UTC, run 5)

- Orient: no open `[G]` qa issues (#141 fixed). #160 (apple, not mine) had iPad still running; #73 (docs) conflicts; left alone.
- G0 slice 1 (PR agent/map-view-menu): "Map" button (top-left of the map card) opens `MapMenu.qml`: per-layer on/off for every layer in `MapOverlays` (independent of panes), night shade, DE/DX path, label mode (layer/None/Prefix/Call/Dot). State lives in `src/map/view_options.*` (hidden layers stored by id so a layer hidden before it is published stays hidden), saved in options.ini `[map]` and restored at start. Shared files touched minimally: `AppSettings::optionsPath()`, `CMakeLists.txt`/`HamClockApp.cmake` lists, Main.qml (the header Hide/Show path button now drives `worldMap.showPath`, which persists). Test: `qt-map-view-options-regression` (apply, round trip, defaults, night shade render). Offscreen 800x480 and 1024x600 screenshots checked with the menu closed (CLI cannot open it); open menu not yet screenshotted. Not validated on device.
- Next: G0 slice 2: base style/rotation, grid and projection entries, tap marker details and Set DE/DX popup.

## 2026-10-14 (UTC, run 6)

- Orient: merged #167 (E4, green, auto-merge). #168 (not mine) still had macOS/iPad running; #73 (docs) conflicts; left alone. No open `[G]` qa issues (#152 items 7/8 are map-related: label collision fixed earlier by declutter; restart-zoom state is not persisted by the map today, so a restart shows DE/DX).
- G0 slice 2 (PR agent/map-tap-popup): tapping the map opens `MapTapMenu.qml` (lat/lon, Set DE, Set DX, Cancel) saving through `AppSettings::saveStation` (callsign, zone and the other point kept; save errors shown). Shared files: Main.qml (map card, ~9 lines), `cmake/HamClockApp.cmake` QML list. No new C++; `saveStation` is already covered by existing tests, popup verified by `qml-app-smoke` and an offscreen 800x480 screenshot with the popup forced open (fits, buttons tappable). Not validated on device.
- Next: G0 slice 3: marker tap details + Set DX, base style/rotation, grid and projection entries.
