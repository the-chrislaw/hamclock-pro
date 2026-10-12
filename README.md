# HamClock Pro

Original native Raspberry Pi dashboard foundation. Development preview only: this is not yet a HamClock replacement and does not establish feature parity with original HamClock 4.22.

## Build

Requires a C++17 compiler, GNU make, SDL2 development headers/library, and `sdl2-config`. On Raspberry Pi OS, use distribution packages `build-essential` and `libsdl2-dev`.

```sh
make
make check
make integration-check
make map-check
make preview
./build/hamclock-pro
```

The application uses a resizable native window with an 800 × 480 logical canvas. Escape closes it. Pane values remain illustrative; the map now uses real Natural Earth coastlines in an equirectangular projection. No radio control, incoming network service, live data provider, telemetry, or privileged runtime is implemented.

Use Tab/Shift-Tab or Left/Right to focus a pane, then Enter or Space to cycle its illustrative selection. A visible outline marks focus. Mouse selection remains available. These controls do not provide a native screen-reader interface; the accessibility-tree work remains pending.

The native window still uses the earlier experimental visual style, which was rejected for the product. The accepted softer design is in `design/dashboard.html`; open it locally to inspect System/Light/Dark appearance, text enlargement and touch controls. It contains illustrative readings and is not the native runtime.

## Targets and validation

First user test target: Pi 4 with a seven-inch touchscreen. Pi 3 remains a required target; Zero 2 W is an intended target and original Zero W is exploratory. Local compilation is not evidence of Raspberry Pi compatibility or performance. Each supported board needs real display, input, startup, CPU and memory measurements before release.

The accepted HTML design study offers System (default), Light and Dark appearance. System follows the computer preference. Larger text and optional touch controls are available in the study; these are not yet ported to the native application. Local browser control is requested but not implemented; SSH is an operating-system administration path, not an application feature.

## First station calculations

Original offline calculations now provide Maidenhead locators, spherical great-circle distance and initial short/long path bearings. They are available through the command line, not yet through the station editor:

```sh
./build/hamclock-pro --calculate 37.7749 -122.4194 51.5074 -0.1278
```

Inputs are signed decimal latitude/longitude in degrees. Invalid or non-finite coordinates are rejected. Coincident and antipodal locations return an explicit status and null bearings. Distances use a mean-radius spherical Earth approximation; they are not surveyed ellipsoidal distances. Domain and input validation checks run with `make check`.

Station configuration can also be saved and inspected explicitly:

```sh
./build/hamclock-pro --save-station ./station.conf N0CALL 0 0 51.5 -0.1 system
./build/hamclock-pro --load-station ./station.conf
```

Use a private directory you control; a valid save replaces the selected regular file. Launch `./build/hamclock-pro --station ./station.conf` to load that explicit file into the native display. It shows callsign, DE/DX locators, spherical distance, initial true bearing and a sampled short great-circle route. A separate Qt station-editor prototype is in local validation; appearance is not applied by this experimental SDL renderer. Invalid/corrupt input produces an error before opening the display instead of silently replacing it with defaults. See `docs/station-settings.md` for format, file permissions, atomic-save behavior and limitations. `make integration-check` uses Python 3 to exercise the actual executable, including failure preservation; Python is not required by the application at runtime.

For a reproducible offline station frame, use `./build/hamclock-pro --station ./station.conf --headless-frame ./build/station.bmp`. Coincident/antipodal bearings are explicitly undefined; antipodal routes are not drawn. A route represents geometry, not predicted propagation or radio coverage.

The station-edit draft model supports bounded intermediate input, strict validation and cancel without saving. It is exercised by `make check` and is wired to the separate Qt dialog prototype. Its callsign policy remains provisional; the final-original manual must define parity rather than treating these input limits as original behavior.

SDL2 is the installed renderer used for this first executable experiment. Framework selection remains provisional until target-hardware measurements are available. Do not add dependencies, external graphics or fonts without recording their origin and license.

See `docs/` for review findings and feature work. Original application source is MIT licensed; SDL2 and system components retain their own licenses. Distribution dependency notices and name clearance remain release gates.

Bundled Natural Earth 110m coastline data are public domain, separately from the application's MIT copyright. Source version, checksum, transformations and reproduction instructions are in `docs/map-provenance.md`. The map is a generalized global coastline, not a navigation chart; other HamClock projections, filled land, terminator and station overlays remain pending.

`make verify` runs the native unit checks, executable regression checks, and offline malformed-input checks for the coastline importer. The importer tests create synthetic files in temporary directories and do not download geographic data. UI and Pi hardware checks remain separate.

## Optional Qt station editor (development prototype)

Requires approved Qt 6.4 or later Core/Gui/Widgets development packages and CMake. The SDL build remains independent.

```sh
cmake -S . -B build/qt -DCMAKE_PREFIX_PATH=/path/to/qtbase
cmake --build build/qt
ctest --test-dir build/qt --output-on-failure
```

On macOS the binary is `build/qt/hamclock-pro-editor.app/Contents/MacOS/hamclock-pro-editor`; other platforms use `build/qt/hamclock-pro-editor`. Pass `--station PATH` to edit a valid file or `--new-station PATH` for an unsaved draft. Save validates and persists; Cancel discards edits. New-file saves refuse any existing destination. Use a private directory you control.

This editor has no dashboard or on-screen keyboard. Actual native Save, invalid-input and Cancel checks have passed locally; theme repair, rendered-widget regressions and independent review now pass. See `docs/qt-editor.md` and `docs/computer-validation.md`. Qt and its dependencies retain separate licensing obligations; distribution packaging, assistive-technology and Pi testing remain release gates.

The same optional CMake build now also provides `hamclock-pro-map-preview --station PATH` (inside its `.app/Contents/MacOS/` directory on macOS). This read-only native preview shows real coastlines, saved station grids, calculated short-path distance/bearing and a seam-safe route. System/Light/Dark and Show short path affect the preview only. Its map/keyboard/theme behavior was computer-tested locally; Pi, touch and full accessibility validation remain pending.

## Softer native station dashboard (development increment)

The optional CMake build also produces `hamclock-pro-dashboard --station PATH` (inside its `.app/Contents/MacOS/` directory on macOS). Use an existing valid private station file. This native slice shows actual UTC, saved DE/DX coordinates and grids, computed short-path readings, and licensed real coastlines. Edit station opens the shared dialog; successful Save refreshes the dashboard and saved System/Light/Dark appearance. Cancel and validation/write failures preserve the current station.

Local computer-control checks and independent unit/executable audits passed for this increment. It is not the full accepted dashboard or final-original feature parity; Pi/touch, deployment licenses, assistive-software and other release checks remain pending. Details: `docs/qt-dashboard.md`, `docs/qt-dashboard-audit.md`, `docs/computer-validation.md`.

The native dashboard now follows the approved grouped home-screen composition: station/UTC sidebar, top instrument strip, and a dominant map card in warm Light or slate Dark. Instrument providers that are not implemented are explicitly marked unavailable. The masthead appearance control previews System/Light/Dark without saving; persist your preference through Edit station. Keyboard route visibility, Save/Cancel/error flows and compact/enlarged native screens were computer-tested locally. This is a partial implementation, with real unfilled coastlines and no live instrument service. See `docs/native-shell-audit.md` and `docs/computer-validation.md`.

`make land-check` runs 27 offline synthetic polygon-import regressions, also included in `make verify` and CTest. The original build-time `tools/import_land.py ARCHIVE OUTPUT` accepts a bounded offline ZIP with one root `ne_110m_land.shp`, preserves explicit rings/records and winding, and emits C++ geometry with the source digest. It does not download data or classify/repair topology. No real land asset or filled-map renderer is included yet; actual asset verification and geometric rendering are separate gates. Python 3 is required for development tests (CMake discovers it only when BUILD_TESTING is enabled), not the operational dashboard. See `docs/land-import-audit.md`.
