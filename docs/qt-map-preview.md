# Native station map preview

`hamclock-pro-map-preview --station PATH` loads and validates an explicitly supplied existing station configuration **before** creating QApplication. Missing/malformed/unknown settings cause exit 2 with a diagnostic; no default/demo station is substituted. Other syntax or no arguments prints usage and exits 2. Qt receives only the executable name rather than interpreting user-selected plugin options.

The resizable 800 × 480 dialog shows the actual loaded callsign and DE/DX Maidenhead grids, computed spherical short-path distance/bearing, and the shared `WorldMapWidget` with real Natural Earth coastline, distinct DE/DX marker shapes and seam-split short path. Coincident/antipodal route states are expressed by the shared map's route summary. It is a native map preview, not the completed dashboard or full HamClock replacement.

Standard Appearance and Show short path controls have accessible names/descriptions, defined tab order and 44 logical-pixel minimum height. Labels use explicit plain-text format and word wrapping. Appearance starts from the saved System/Light/Dark choice, applies the shared dialog helper and updates the map's mode. Route visibility affects only the drawn route; endpoints remain. Closing/Escape exits without writing settings. Every change in this preview is temporary, which is stated visibly.

No configuration writes, network calls, editor launch, live radio/data providers or fabricated readings are implemented here. The coastline credit and offline state are visible. Exact startup/resize/touch/system-theme behavior, screen-reader interaction and Pi resource use need independent computer/hardware validation. Original application source remains MIT; Qt and geographic data retain their separate reviewed terms.

Root build integration should link this source with `editor_appearance.cpp`, `world_map_widget.cpp`, original station view/settings/geometry and map projection/path sources against dynamic Qt Core/Gui/Widgets. Required tests: missing/invalid station rejected before GUI, known station grids/route match CLI, Light/Dark/System changes update the map and surrounding surface, route toggle hides only route, Escape leaves source bytes unchanged, and resized/enlarged layouts remain usable. No complete accessibility compliance, Pi benchmark or final-4.22 parity claim is made.

## Local validation

The integrated CMake target builds with dynamic Qt Core/Gui/Widgets. Independent review, both Qt regression targets, 157 map widget checks, sanitizers and make verify pass. Native Mac observation verified real coastlines, the saved sample grids/distance/bearing/route, Light/Dark surfaces, route hiding with endpoints retained, corrected actual-value accessibility labels and application-local all-control keyboard focus. Tab/Space/Shift-Tab/Escape were exercised; input configuration bytes were unchanged. See computer-validation.md for untested cases and limits. This does not establish full screen-reader, Pi or touch support.

Only the installed Qt 6.11.2 build was validated. The CMake minimum remains 6.4, but target Qt 6.4 compilation, plugin integration and Pi behavior need verification before claiming compatibility.
