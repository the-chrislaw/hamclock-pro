# Developing HamClock Pro

Build, test and command-line notes moved out of the README.

## Build and test

Requires CMake 3.16+, a C++17 compiler, Qt 6.4+ (Core, Gui, Widgets) and Python 3 for the development tests. On Raspberry Pi OS or Ubuntu: `sudo apt install build-essential cmake qt6-base-dev python3`. On macOS with Homebrew: `brew install cmake qt`.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

CI runs the same build and tests on Ubuntu, plus an AddressSanitizer/UBSan build (`.github/workflows/ci.yml`). Local compilation is not evidence of Raspberry Pi compatibility or performance.

## Programs

On macOS the GUI programs are app bundles, e.g. `build/hamclock-pro.app/Contents/MacOS/hamclock-pro`; elsewhere they are plain binaries in `build/`.

| Program | Purpose |
|---|---|
| `hamclock-pro` | The Qt Quick dashboard (`--help` lists options). |
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

## Screenshots

README images are rendered offline with fixtures (the data shown is recorded sample data, some of it days old, and the panes mark it Stale):

```sh
build/hamclock-pro --config-dir <tmp> --fixtures <dir with widget fixtures> --theme dark --size 1024x600 --screenshot docs/images/pi-7inch-dark.png
```

Run with `QT_QPA_PLATFORM=offscreen` on a machine without a display. Do not describe a screenshot as a device capture.
