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

## 2026-10-14 (UTC, run 7)

- Orient: no open `[G]` qa issues; no open PRs of mine. #176/#177 (wx/apple) and #73 (docs, conflicts) left alone.
- G0 slice 3 (PR agent/map-menu-styles): Map menu gets "Base map" (Plain + each published style) and "Rotate styles" (Off/20/60/90/120 s), hidden until a style is published. `MapViewOptions` gains `style` and `rotation_s` (options.ini `[map] style/rotation`, unknown intervals rejected); `MapItem` pushes them into the `MapStyles` registry and lets styles published after start join the rotation. Test: extended `qt-map-view-options-regression`. No styles are published yet, so the new menu rows are not visible in screenshots. Not validated on device.
- Next: G0 slice 4: marker tap details + Set DX, grid and projection entries (G4/G5).

## 2026-10-14 (UTC, run 8)

- Orient: no open `[G]` qa issues; no open PRs of mine. #185/#186 (not mine) had CI still running so were not merged; #73 (docs) left alone.
- G0 slice 4 (PR agent/map-grid): `src/map/grid.hpp` (`MapGrid` None/Tropics/Lat-Lng/Maidenhead, pure `grid_lines`), drawn by `MapRenderer` under the night shade; `MapViewOptions.grid` (options.ini `[map] grid`, out-of-range rejected); `MapItem.grid`; Map menu "Grid" row. Test: extended `qt-map-view-options-regression` (line coordinates/counts, persistence, render differs). Offscreen 1024x600 dark with Maidenhead grid checked. Local ASan/UBSan run not done (left to the CI asan-ubsan job). Not validated on device.
- Next: G0 slice 5: marker tap details + Set DX; projection entry (G4); azimuth rings and CQ/ITU zones (G5, need dataset licence record).

## 2026-10-14 (UTC, run 9)

- Orient: no open `[G]` qa issues. #186 (apple, not mine) still had macOS/iPad queued so was not merged; #73 (docs) not mergeable; left alone.
- G0 slice 5 (PR agent/map-marker-tap): tapping an overlay marker opens the tap popup with the marker's call and optional `OverlayMarker::detail` line plus Set DE / Set DX at the marker position. New pure `hit_test_marker` (visible layers only, nearest within 14 px x ui scale, world copies either side checked); `MapItem::markerTapped`. Shared file: Main.qml (one handler). Test: extended `qt-map-overlay-regression`. 1024x600 offscreen render unchanged (CLI cannot open the popup). ASan/UBSan left to CI. Not validated on device.
- Next: G0 slice 6/G4 projection entry (Mercator centre longitude, azimuthal, Robinson); G5 azimuth rings and zones (need dataset licence record).

## 2026-10-14 (UTC, run 10)

- Orient: no open `[G]` qa issues; no open PRs of mine. #201 (apple, not mine) CI still running, #73 (docs) left alone.
- G5 slice (PR agent/map-azimuth-rings): Map menu Grid gains "Rings": great-circle distance rings every 30 degrees of arc (3336 km) around DE (`ring_points`, pure; seam-split by the renderer; only drawn when DE is set). `MapGrid::AzimuthRings = 4`, persisted in options.ini `[map] grid`. Test: extended `qt-map-view-options-regression` (every ring point at the exact arc distance, render differs with DE). Screenshot fixtures have no DE so rings are not visible there; covered by the render test. ASan/UBSan left to CI. Not validated on device.
- Next: projection entry (Mercator centre longitude, azimuthal, Robinson); CQ/ITU zones (need dataset licence record).

## 2026-10-14 (UTC, run 11)

- Orient: no open `[G]` qa issues; no open PRs of mine. #207/#210 (not mine) CI still running, #73 (docs) left alone.
- G4 slice (PR agent/map-projection): Map menu "Centre": DE / Greenwich / Americas / Asia / Pacific (options.ini `[map] center`, out-of-range rejected). `MapScene::center_lon` feeds the viewport as the preferred pan; `MapViewport::fill` now honours a pan/centre longitude even when the world already fits the card width (neighbouring world copies are drawn), so a drag at 1x also rotates the world. Setting a centre resets zoom/pan. Test: extended `qt-map-view-options-regression` (round trip, bad value, DE default, fixed centre on 1024x600 and an 800x310 card). Offscreen 1024x600 dark with `center=-90` checked: overlays line up with land. ASan/UBSan left to CI. Not validated on device.
- Next: azimuthal and Robinson projections; CQ/ITU zones (need dataset licence record).

## 2026-10-14 (UTC, run 12)

- Orient: no open `[G]` qa issues; no open PRs of mine. #220/#221 (not mine) CI incomplete, #73 (docs, conflicts) left alone.
- G5 slice (PR agent/map-sky-points): Map menu toggle "Moon / antipode" (options.ini `[map] sky_points`, default off) draws the sublunar point (open circle) and the DE antipode (X). New pure `src/map/sky_points.hpp` (`sublunar_point` from `moon_position` RA/Dec minus GMST, `antipode`). Shared file: `CMakeLists.txt` adds `src/domain/astro_moon.cpp` to `hamclock-map-core` (the sunmoon/eme widgets also compile it; full build links fine). Test: extended `qt-map-view-options-regression` (antipode wrap, GMST at J2000, Moon near zenith at the sublunar point via `moon_horizontal`, render differs, persistence). Offscreen 800x480/1024x600/1024x630/1920x1080 dark checked: marker visible, lines up. Full ctest 132/132 locally; ASan/UBSan left to CI. Not validated on device.
- Next: azimuthal and Robinson projections (renderer is equirectangular-only; needs a projection abstraction first); CQ/ITU zones (need dataset licence record); long path; gray mode.

## 2026-10-14 (UTC, run 13)

- Orient: no open `[G]` qa issues; no open PRs of mine. #73 (docs) and #227 (apple, CI running) not mine, left alone.
- G5 slice (PR agent/map-long-path): Map menu toggle "Long path" (options.ini `[map] long_path`, default off) draws the DE/DX route the long way round the same great circle. New pure `map_long_path` (shares the short-path implementation, arc 2*pi - short arc, exact endpoints, antipodal rejected). Tests: extended `path_tests` (endpoints, summed length equals `long_distance_km`, equator heads west) and `qt-map-view-options-regression` (round trip, render differs). Full ctest 132/132 locally; no screenshot (fixtures have no DE/DX path); ASan/UBSan left to CI. Not validated on device.
- Next: azimuthal and Robinson projections (needs a projection abstraction); CQ/ITU zones (need dataset licence record); gray mode.

## 2026-10-14 (UTC, run 14)

- Orient: no open `[G]` qa issues; no open PRs of mine. #234/#235 (not mine) macOS CI still running, #73 (docs) left alone.
- G5 slice (PR agent/map-gray): Map menu toggle "Gray map" (options.ini `[map] gray`, default off) renders sea, land and coast in grayscale (`qGray`), overlays keep their colours. `MapScene::gray`, `MapViewOptions::gray`, `MapItem.gray`. Test: extended `qt-map-view-options-regression` (default, round trip, render differs). Full ctest 132/132 locally; ASan/UBSan left to CI. Not validated on device.
- Next: azimuthal and Robinson projections (needs a projection abstraction); CQ/ITU zones (need dataset licence record).

## 2026-10-14 (UTC, run 15)

- Orient: no open `[G]` qa issues; no open PRs of mine. #73 (docs), #241 (macOS universal build still running) and #242 (no checks yet), none mine, left alone.
- G4 slice (PR agent/map-projection-math): pure `map_project_azimuthal`/`map_unproject_azimuthal` (DE-centred equidistant, unit disc, antipode on the rim) and `map_project_robinson`/`map_unproject_robinson` (published 5-degree table, linear between rows) in `src/map/projection.*`. No renderer or menu change yet (renderer is equirectangular-only; needs a projection abstraction in the next slice). Tests (in `run_map_tests`): centre, cardinal points, Denver-London distance, round trips, rejection outside disc/outline, Robinson knots and symmetry; the antipode test caught an antipode-returns-centre bug, fixed. Not validated on device; no screenshots (no visual change). ASan/UBSan left to CI.
- Next: renderer projection abstraction + Map menu "Projection" row (Equirectangular / Azimuthal / Robinson); CQ/ITU zones (need dataset licence record).

## 2026-10-14 (UTC, run 16)

- Orient: #247 (owner priority, mine) open; no open PRs of mine. #242/#252/#253 (not mine) macOS/iPad checks still queued/running so not merged; #73 (docs) left alone.
- G4 slice 1 of #247 (PR agent/map-projection-render): Map menu "Projection" row (Flat / Azimuthal / Robinson, options.ini `[map] projection`, bad value rejected). New `MapFrame` (`projection.*`): fit + `to_pixel`/`to_location`, the one transform shared by the base map, DE/DX, sun/moon, route, rings, grid, overlay markers, marker hit-test and taps (`MapItem::mouseReleaseEvent`). The vector layer code was extracted into `paint_geo_layers` (flat map unchanged, now also used by projected maps); the projected base map is resampled per pixel (bilinear, cached) from a cached 2048x1024 equirectangular image, night shade computed per pixel from the subsolar point, so land and overlays cannot drift apart. Azimuthal centres on DE (else the Centre choice); Robinson honours the Centre choice. Zoom/pan/+/- do nothing on these projections (reset on switch); pinch/wheel zoom is slice 2 of #247. Tests: `run_map_tests` (frame fit/round trip/centre/outline for several card sizes) and `qt-map-view-options-regression` (persistence, render differs, DE marker at the frame position, marker hit). Full ctest 135/135 locally; 800x480/1024x600/1024x630/1920x1080 renders checked (overlays line up with land). ASan/UBSan left to CI. Not validated on device.
- Next: #247 slice 2 (PinchHandler/trackpad/wheel/double-tap zoom); Azimuthal-one; CQ/ITU zones (need dataset licence record).

## 2026-10-14 (UTC, run 17)

- Orient: #247 slice 2 (mine) open; no open PRs of mine. #259 (apple, not mine) macOS/iPad checks still queued so not merged; #73 (docs) left alone.
- G0 gestures slice 2 of #247 (PR agent/map-gestures): zoom about the gesture point on the flat map: mouse wheel / trackpad scroll (`wheelEvent`, 1.0015^delta), double tap/click (2x, back to 1x at max), and a `PinchHandler` in Main.qml (touch and macOS trackpad pinch) all call `MapItem::zoomAt(x, y, factor)`; +/- buttons kept. Pure `MapViewport::center_for_anchor` keeps the location under the finger. Zoom stays flat-map only (projected maps are fixed, by design of slice 1). Test: extended `qt-map-viewport-regression` (anchor stays put on 800x310, 1024x600, 1920x1080). Full ctest 137/137 locally; 1024x600 dark render unchanged; gesture feel not exercised (offscreen) and not validated on device. ASan/UBSan left to CI.
- Next: CQ/ITU zones (need dataset licence record); Azimuthal-one.

## 2026-10-14 (UTC, run 19)

- Orient: no open `[G]` qa issues besides #247 (delivered); no open PRs of mine. #274/#276 (not mine) macOS/iPad checks still running, #73 (docs) left alone.
- G5 slice (PR agent/map-borders): Map menu toggle "Country borders" (options.ini `[map] borders`, default off). New dataset Natural Earth 110m admin-0 boundary lines (public domain, provenance and SHA in `docs/map-provenance.md`), converted by `tools/import_borders.py` into `src/map/borders.hpp` (generated, 3108 points). Borders are drawn dashed and dim into the cached 2048x1024 base image, so they work on the flat map and on azimuthal/Robinson through the same `MapFrame` transform, and cannot drift from the land. Tests: `run_map_tests` (dataset bounds, coverage, US-Canada border near 49N) and `qt-map-view-options-regression` (default, round trip, render differs on flat and Robinson). Not validated on device.
- Next: US states and cities (licensed data), CQ/ITU zones (need dataset licence record), Azimuthal-one.

## 2026-10-14 (UTC, run 18)

- Orient: #262 (label collisions, qa) is first; no open PRs of mine. #268 (apple, not mine) and #73 (docs) left alone.
- #262 (PR agent/map-label-collisions): labels now avoid the map buttons (`MapScene::reserved`, top-left Map button and top-right +/-/reset, sized from `ui_scale`), the route line (`line_obstacles` along the drawn path, off-map jumps skipped) and every marker symbol of every layer (all symbols are drawn before any label). Layers place labels by priority (`overlay_label_priority`: satellites, then spots/others, then quakes/fires/storms). DE/DX labels try four corners and take the first clear of the path. Tests: extended `qt-map-overlay-regression` (priority order, obstacle rects, label on a line dropped). Full ctest 138/138 locally; 1920x1080 and 1024x600 dark renders checked (fixtures have no DE so no path in the shot). ASan/UBSan left to CI. Not validated on device. Shared file note: none outside owned paths except the test.
- Next: CQ/ITU zones (need dataset licence record); Azimuthal-one.

## 2026-10-14 (UTC, run 20)

- Orient: no open `[G]` qa issues besides #247 (delivered); no open PRs of mine. #291/#292 (not mine) macOS/iPad checks still running so not merged; #73 (docs) left alone.
- G4 slice (PR agent/map-azimuth-one): Map menu Projection gains "Azim one" (`MapProjection::AzimuthalOne = 3`, options.ini `[map] projection=3`): the DE-centred equidistant disc scaled so the rim is 90 degrees of arc, near hemisphere only; far-hemisphere points have no pixel (markers/route skipped, taps outside the disc ignored). Same `MapFrame` transform for base map, overlays and taps. Tests: `run_map_tests` (centre, exactly 2x the azimuthal scale, antipode/far hemisphere dropped, round trips, several card sizes) and `qt-map-view-options-regression` (persistence, render differs, DE marker at frame position). Full ctest 142/142 locally; 800x480/1024x600/1920x1080 dark renders checked (fixtures have no DE so it centres on 0,0). ASan/UBSan left to CI. Not validated on device.
- Next: CQ/ITU zones (need a polygon dataset with a recordable licence; none confirmed yet), US states and cities.

## 2026-10-14 (UTC, run 21)

- Orient: no open `[G]` qa issues besides #247 (delivered); no open PRs of mine. #73 (docs, not mine) has merge conflicts; #297/#299/#300/#301 (not mine) macOS/iPad checks still running, left alone.
- G5 slice (PR agent/map-us-states): Map menu toggle "US state lines" (options.ini `[map] states`, default off). New dataset Natural Earth 110m admin-1 state/province lines (public domain; provenance and SHA in `docs/map-provenance.md`), generated into `src/map/states.hpp` (903 points) by `tools/import_borders.py --states`. Drawn dotted into the cached base image like country borders, so it works on flat and projected maps through the same `MapFrame` transform. Tests: `run_map_tests` (dataset bounds, coverage, a state line near 41N) and `qt-map-view-options-regression` (default, round trip, render differs on flat and Robinson). Full ctest 142/142 locally; 1024x600 and 800x480 dark renders checked. ASan/UBSan left to CI. Not validated on device.
- Next: cities (licensed data), CQ/ITU zones (need a polygon dataset with a recordable licence; none confirmed yet).

## 2026-10-14 (UTC, run 22)

- Orient: no open `[G]` qa issues besides #247 (delivered); no open PRs of mine. Merged #307 (not mine, all checks green). #308 (CI still running) and #73 (docs, conflicts) left alone.
- G5 slice (PR agent/map-cities): Map menu toggle "Cities" (options.ini `[map] cities`, default off). New dataset Natural Earth 110m populated places (public domain; provenance and SHA in `docs/map-provenance.md`), generated into `src/map/cities.hpp` (243 cities) by `tools/import_cities.py`. Drawn in the vector layer (dot + name, largest first, after all overlay markers/labels, skipping any that collide with markers, labels, route or map buttons), so it works on flat and projected maps through the same `MapFrame` transform. Test: extended `qt-map-view-options-regression` (default, round trip, render differs on flat and Robinson). Full ctest 144/144 locally; 1024x600 and 800x480 dark renders checked (labels clear markers). ASan/UBSan left to CI. Not validated on device.
- Next: CQ/ITU zones (need a polygon dataset with a recordable licence; none confirmed yet) - consider an "Owner input needed" issue if none is found.

## 2026-10-15 (UTC, run 23)

- Orient: no open `[G]` qa issues besides #247 (delivered); no open PRs of mine. #396 (not mine) macOS/iPad checks still running so not merged; #73 (docs) left alone. CQ/ITU zones stay blocked on #353 (owner input).
- G0 slice (PR agent/map-marker-detail): the marker tap popup now also shows "From DE: <km>, <bearing>" via new pure `overlay_path_text` (spherical `geometry`; distance only for same place/antipode; empty without DE) exposed to QML as `MapItem::pathText`. Test: extended `qt-map-overlay-regression`. Full ctest 154/154 locally; 1024x600 dark render loads with no QML errors (popup itself not screenshotted: offscreen has no tap). ASan/UBSan left to CI. Not validated on device.
- Next: waiting on #353 for CQ/ITU zones.

## 2026-10-16 (UTC, run 24)

- Orient: open `[G]` qa issue #423 (planet labels overprint); no open PRs of mine; #73 (docs, not mine) left alone. CQ/ITU zones stay blocked on #353.
- #423 (PR agent/map-planet-labels): re-rendered 1920x1080 dark on current main; Venus's label is already dropped and Mercury's kept, no overprint, because the #262 collision work places every label against all marker symbols and earlier labels. Added a regression test (two bodies 40 px apart never overprint) to `qt-map-overlay-regression`. No renderer change needed. Full map ctest passes locally; ASan/UBSan left to CI. Not validated on device.
- Next: waiting on #353 for CQ/ITU zones.
