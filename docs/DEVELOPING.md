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

## Mac App Store / TestFlight (macOS)

The `macOS TestFlight upload` job in `.github/workflows/apple.yml` builds the native macOS app (universal, not the iPad app on Mac) for the Mac App Store and uploads it to TestFlight on the same App Store Connect record as the iPad app (app id 6822478434, bundle id `com.chrislaw.hamclockpro`, universal purchase). Like the iPad upload it runs only on pushes to `main` while the repository variable `TESTFLIGHT_UPLOADS` is `true`, and it skips cleanly while any secret below is missing. The Developer ID dmg (`macOS universal build`) is unchanged and keeps bundle id `org.hamclockpro.app`.

What the job does: builds with `-DHAMCLOCK_MACOS_BUNDLE_ID=com.chrislaw.hamclockpro -DHAMCLOCK_BUILD_NUMBER=<run number>`, deploys Qt with `macdeployqt -appstore-compliant`, creates a temporary `MAC_APP_STORE` provisioning profile through the API (`packaging/apple/asc_profile.py`), embeds it, signs with **Apple Distribution** and the App Sandbox entitlements in `packaging/apple/AppStore.entitlements` (network client, network server for the WSJT-X/N1MM UDP listener, user-selected read-only files and app-scoped bookmarks for the ADIF log), packages a `.pkg` signed with the **Mac Installer Distribution** identity, and uploads it with `xcrun altool` using the existing App Store Connect API key.

Sandboxed behaviour: settings and caches go to `~/Library/Containers/com.chrislaw.hamclockpro/Data/Library/...` (Qt's standard paths resolve there automatically, so the first sandboxed launch starts with fresh settings). Account passwords and API keys are kept in the Keychain on macOS and iPadOS (moved from `credentials.ini` on first start; runs with `--config-dir` keep them in that folder). The ADIF log must be picked with **Choose…** in Settings; the app keeps a read-only security-scoped bookmark so it can read the log after a relaunch. A typed path is not readable under the sandbox. The `macOS universal build` job runs a sandboxed launch (ad-hoc signed with the same entitlements) on every PR.

### One-time owner steps

1. **Add macOS to the App Store record.** In App Store Connect, open HamClock Pro → Apps sidebar → "+" next to the platform list (or *Add Platform*) → **macOS**. This creates the macOS version on the same record, which universal purchase needs. Fill in the macOS App Information (category Utilities) when asked.
2. **Check the bundle id covers macOS.** In Certificates, Identifiers & Profiles → Identifiers → `com.chrislaw.hamclockpro`, the platform should include macOS (identifiers created since 2020 are shared across platforms). If profile creation in the job fails with a platform error, enable macOS there.
3. **Create a Mac Installer Distribution certificate.** Certificates → "+" → *Mac Installer Distribution*. Create the CSR with Keychain Access (Certificate Assistant → Request a Certificate From a Certificate Authority, saved to disk), upload it, download the `.cer`, double-click to add it to your login keychain, then in Keychain Access → My Certificates select "3rd Party Mac Developer Installer: …" (or "Mac Installer Distribution: …") → Export as `.p12` with a password.
4. **Add the two new secrets** (you type the values; nothing echoes them):

   ```sh
   base64 -i installer.p12 | gh secret set MAC_INSTALLER_CERT_P12_BASE64
   gh secret set MAC_INSTALLER_CERT_PASSWORD
   ```

   The app itself is signed with the existing Apple Distribution certificate (`IOS_DIST_CERT_P12_BASE64` / `IOS_DIST_CERT_PASSWORD`; Apple Distribution certificates work for every platform) and the existing `ASC_KEY_ID`, `ASC_ISSUER_ID`, `ASC_KEY_P8_BASE64` and variable `APPLE_TEAM_ID`.
5. **Upload a build** the same way as for the iPad: set `TESTFLIGHT_UPLOADS` to `true` for the run you want, then back to `false`. The macOS build then appears under TestFlight → macOS. Add yourself as an internal tester and install it with the TestFlight app on the Mac.
