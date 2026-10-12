# HamClock Pro

Original native Raspberry Pi dashboard for amateur radio: world time, station geometry, map, and (planned) space weather and spots. Development preview only: this is not yet a HamClock replacement and does not establish feature parity with original HamClock 4.22. See [docs/ROADMAP.md](docs/ROADMAP.md) for the plan to 1.0.

## Build and test

Requires CMake 3.16+, a C++17 compiler, Qt 6.4+ (Core, Gui, Widgets) and Python 3 for the development tests. On Raspberry Pi OS or Ubuntu: `sudo apt install build-essential cmake qt6-base-dev python3`. On macOS with Homebrew: `brew install cmake qt`.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

CI runs the same build and tests on Ubuntu, plus an AddressSanitizer/UBSan build (`.github/workflows/ci.yml`). Local compilation is not evidence of Raspberry Pi compatibility or performance.

## Programs

On macOS the GUI programs are app bundles, e.g. `build/hamclock-pro-dashboard.app/Contents/MacOS/hamclock-pro-dashboard`; elsewhere they are plain binaries in `build/`.

| Program | Purpose |
|---|---|
| `hamclock-pro-dashboard --station PATH` | Main native dashboard: UTC, DE/DX, real map with land fill, Big Clock, stopwatch, countdown. Edit station saves through the shared dialog. |
| `hamclock-pro-editor --station PATH` or `--new-station PATH` | Standalone station editor. Save validates and persists; Cancel discards. New-file saves refuse an existing destination. |
| `hamclock-pro-map-preview --station PATH` | Read-only map preview with station grids and short-path route. |
| `hamclock-pro-cli` | Offline command-line tools (below). |

Use a private directory you control for station files. Instrument panes without a data provider are explicitly marked unavailable; no simulated reading is presented as live. No radio control, incoming network service, live data provider or telemetry is implemented.

### Command-line tools

```sh
./build/hamclock-pro-cli --calculate 37.7749 -122.4194 51.5074 -0.1278
./build/hamclock-pro-cli --save-station ./station.conf N0CALL 0 0 51.5 -0.1 system
./build/hamclock-pro-cli --load-station ./station.conf
./build/hamclock-pro-cli --self-test
```

`--calculate` prints JSON with Maidenhead locators, spherical great-circle distance and initial short/long path bearings. Inputs are signed decimal degrees; invalid or non-finite coordinates are rejected. Coincident and antipodal locations return an explicit status and null bearings. Distances use a mean-radius spherical Earth, not surveyed ellipsoidal distances. A route represents geometry, not predicted propagation.

A valid save atomically replaces the selected regular file with mode 0600; symlinks and corrupt files are refused without modification. See `docs/station-settings.md` for the format and limitations.

## Targets

First user test target: Pi 4 with a seven-inch touchscreen. Pi 3 is a required target; Zero 2 W is intended and original Zero W exploratory. Each supported board needs real display, input, startup, CPU and memory measurements before release.

The accepted visual design is `design/dashboard.html` (System/Light/Dark, larger text, touch controls). The native dashboard follows its composition but does not yet match it fully; see `docs/native-approved-design-gap.md`.

## Data, licensing and provenance

Application source is MIT licensed. Qt and system components retain their own licenses; distribution dependency notices and name clearance remain release gates. Do not add dependencies, external graphics or fonts without recording their origin and license.

Bundled Natural Earth 110m coastline and land data are public domain, separately from the application's MIT copyright. Source version, checksum, transformations and reproduction instructions are in `docs/map-provenance.md` and `docs/land-provenance.md`. The build-time importers `tools/import_coastline.py` and `tools/import_land.py` work offline from a supplied archive; their malformed-input regressions run in CTest and never download data. The map is generalized, not a navigation chart.

Review findings, specs and validation notes for each feature are in `docs/`.
