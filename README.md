# HamClock Pro

Original native Raspberry Pi dashboard foundation. Development preview only: this is not yet a HamClock replacement and does not establish feature parity with original HamClock 4.22.

## Build

Requires a C++17 compiler, GNU make, SDL2 development headers/library, and `sdl2-config`. On Raspberry Pi OS, use distribution packages `build-essential` and `libsdl2-dev`.

```sh
make
make check
make preview
./build/hamclock-pro
```

The application uses a resizable native window with an 800 × 480 logical canvas. Escape closes it. Displayed demonstration values and schematic graphics are not live station data. No radio control, incoming network service, data provider, telemetry, or privileged runtime is implemented.

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

SDL2 is the installed renderer used for this first executable experiment. Framework selection remains provisional until target-hardware measurements are available. Do not add dependencies, external graphics or fonts without recording their origin and license.

See `docs/` for review findings and feature work. Original application source is MIT licensed; SDL2 and system components retain their own licenses. Distribution dependency notices and name clearance remain release gates.
