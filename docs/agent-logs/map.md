# Agent log: map

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (UTC)

- Slice 1 of G1 (PR agent/map-purepainter): new QtGui-only `MapRenderer` (`src/map/map_renderer.*`) and pure view maths `MapViewport` (`src/map/viewport.*`). `MapItem` now uses it, so the QML map has no QtWidgets dependency. The map fills its item (issue #79): scale covers the item, polar rows are cropped but never below ±70° (very wide panes wrap longitude instead), tall panes crop longitude around DE, and DE/DX latitudes shift the crop to stay visible. Main.qml: the map now uses `anchors.fill: parent` instead of a forced 2:1 box (shared file, minimal). Land, coast, night shade, subsolar point and DE/DX path are unchanged; tests: new `qt-map-viewport-regression`.
- Screenshots checked offscreen at 800x480, 1024x600, 1920x1080 (light/dark). Not validated on device.
- Not merged by me: PR #73 (docs, not mine) has merge conflicts; left alone.
- Next: switch the legacy `WorldMapWidget` onto `MapRenderer` (then drop QtWidgets from the map library), then G2 projections (Mercator centre longitude, azimuthal, Robinson), zoom, tap Set DE/DX.
