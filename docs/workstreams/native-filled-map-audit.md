# Independent native filled-map source audit

October 11, 2026. Reviewer owns only this report. Prior LF1/LF2 reports stay frozen. Inspected the actual root widget/CMake diff, independent writer regression changes and exact accepted asset/provenance. Built and tested locally; no production source, tests, settings, assets, Git or dependencies changed by reviewer. No native computer control or physical Pi execution occurred in this lane.

## Disposition

**Bounded source/build/regression gate passes; no blocking finding.** Root's separate actual computer-controlled dashboard acceptance remains required before publication. This increment supplies real pinned land fill in the fixed-world equirectangular map, not all original HamClock mapping/projection/layer functionality.

Constructor converts generated point/ring/record arrays into separate correctly typed temporary views, avoiding aliasing casts, then uses the independently reviewed bounded helper to build owned per-record paths once. No geometry decode during painting, runtime file loader, network, new parser or hardware path. Immutable source counts/pins remain unchanged. Invalid bundled geometry fails explicitly; allocation failure can prevent construction, with no allocation-recovery/Pi-memory claim.

Painter clips to existing 2:1 mapBounds before normalized projection. Draw order is land, coastline, route, DE circle/DX square/labels. Save/restore avoids leaking normalized transforms; letterbox follows ocean colors. Existing seam-aware coast/route paths are preserved and not reused to close polygons. System/Light/Dark choice and palette handling remain intact. Description now identifies land and coastline while retaining textual station/route state. This does not establish screen-reader usability or accessibility compliance.

## Executed evidence

- Fresh strict C++17 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan actual-widget build, installed QtCore/Gui/Widgets only, offscreen: **161 existing map checks and 136 filled-widget checks passed**, zero failures/sanitizer diagnostics. Font alias population emits a benign existing missing Sans Serif message; no new dependency installed.
- Filled-widget tests cover800x480/1280x800, explicit Light/Dark and System with both light/dark palettes; continental/polar fill, ocean and Caspian hole, city neighborhoods containing actual fill, route above land, marker centers/outlines above fill, invalid-input rendered-state retention, hide-route restoring land and ocean letterbox. Replaced obsolete coast-only histogram with explicit geographic assertions; existing routing/tiny/appearance tests remain.
- Independent fresh scratch CMake configure/build `/private/tmp/hamclock-resumed-land-audit/native-cmake`: all targets built; **all14CTest tests passed**, zero failures,5.58seconds. Optional Vulkan headers remain absent/nonrequired. Scoped CMake links existing helper into static map library and adds one offscreen regression with15second timeout.
- `make verify` independently passed self-test/CLI regression,31 coast importer tests,31 land importer tests,79,416 locator input checks,155,519 locator boundary checks,152 clock-state checks,85 spot checks,1,566 instrument catalog checks and104 snapshot checks. Duplicate ZIP-member warning comes from a deliberate importer rejection fixture.
- Recomputed sRGB relative-luminance ratios from exact colors: light coast/land3.193, route/land5.034, route/sea5.599; dark coast/land4.502, route/land8.327, route/sea6.206. Low land/sea ratios1.112/1.342 make visible coastline important. These numeric component checks do not certify whole-interface compliance or actual native display contrast.

Prior asset gate reproduced source bytes, exact ten endpoint transformations and fixed geometry; independent full-source containment/raster/contact evidence is in separate reports. No complete cross-record/spherical topology or navigation accuracy claim. Genuine root computer-control screenshots/keyboard interactions remain separate from QImage tests. Actual Pi3/Pi4 memory/startup/frame behavior and Qt6.4 remain untested; Mac performance cannot certify them. No release/public visibility/merge or live HamClock change authorized by this audit.

## Frozen hashes after execution

- `src/qt/world_map_widget.hpp`: `bba8d37ad4c52c97ff59356a56f27b0b743b5b74ff384da15b80b404aeec54fc`
- `src/qt/world_map_widget.cpp`: `5de59c3a1ba149af3f2b7207c967f51e04b00503e9e44606c6b5d2787c5861bf`
- `CMakeLists.txt`: `338dfe6fc1be561b242d9a6af7ea5f3e07b8472c50cffed0afa1d26346db6779`
- `tests/qt_map_regression.cpp`: `e206fff5ab1b9efd67b9509ea6f330691c91bd149ea9eac1a9b400fc8e1f2345`
- `tests/qt_land_widget_regression.cpp`: `e0db29dc9eb79fcd52712ddb2246c9d7f3adc9c5b167bed45cb56e100a77c991`
- `src/map/land.hpp`: `cc2d3904939d2cfe258660b014cdfd19c106a8f65c18dbd034cd13f0f57f72dc`
- `docs/land-provenance.md`: `a3635386f0927902cf6498d7aecb73afd63e010ae0312a9d62e0d2c23df0681c`

Exact next action: root completes actual native computer-control acceptance, records observed failures/untested cases separately, then may publish bounded private draft increment if that gate passes. Remaining widget functionality/provider correctness/hardware/parity gates stay queued.
