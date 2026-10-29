# Apple TV (tvOS) app: design for owner review

Status: **approved by the owner on 2026-10-14** (issue #157: shim approach A, the pane list in section 9, on-screen settings plus iCloud KVS sync, and the `HCP_CORE_WITHOUT_QT` option). Implementation is under way in `apple/tvos/`; see `docs/agent-logs/tvos.md` for progress.

## 1. Goal and constraints

Show the same one-page HamClock dashboard on an Apple TV, readable from a sofa or shack wall at 1080p and 4K. Qt has no tvOS platform plugin, so the UI cannot be the Qt Quick app. Plan: a **native SwiftUI front end** over the **shared C++ core**, built as a static library / XCFramework for `appletvos` and `appletvsimulator`.

- Bundle ID `com.chrislaw.hamclockpro`, shared with the iOS app so tvOS is a platform of the same App Store record (section 10); team via `APPLE_TEAM_ID`. PRs build unsigned for the Simulator; main can upload a signed build to TestFlight (section 11).
- Honest User-Agent on every request: `HamClockPro/<ver> (+https://github.com/the-chrislaw/hamclock-pro)`.
- No private APIs; data-source attribution screen (shared with area F's App Store work).
- Never show stale or invented data as live: every pane carries "as of" age and a Stale/Offline state, same rule as the Qt app.

## 2. Architecture and the C++ core boundary

```
SwiftUI views  ──►  Swift view models (ObservableObject)
                        │  URLSession fetch (Swift)  → raw bytes
                        ▼
              C ABI shim  (hcpcore.h, extern "C", plain structs/UTF-8 strings)
                        ▼
          Qt-free C++ core: parsers, astro/SGP4, models, map projection
```

Swift owns networking, scheduling, caching and rendering. The core owns *interpretation*: bytes in, plain structs out. The boundary is a small C header (`hcpcore.h`) rather than direct Swift/C++ interop, because it is stable across Xcode versions and trivially callable from Swift.

### What is reusable today (audited against `src/`)

| Source | Qt use | tvOS status |
|---|---|---|
| `src/domain/astro_*.cpp/hpp`, `solar_position`, `geometry`, `locator_input`, `clock_state`, `countdown_state`, `stopwatch_state` | none in headers (verify per .cpp) | **reusable as is** |
| `src/domain/astro_sgp4/` (vendored SGP4, MIT) | none | reusable as is |
| `src/map/projection`, `viewport`, `path` | none | reusable as is |
| `src/map/overlay`, `map_renderer`, `styles`, `land_fill_path` | QColor/QPointF/QPainter | needs a shim for geometry types; rendering is redone in SwiftUI Canvas from the same land/coastline tables (`land.hpp`, `coastline.hpp`) |
| `src/widgets/*/*parse*.cpp` (30 files: SWPC, USGS, NHC, NWS, Open-Meteo, POTA/SOTA, PSK, CelesTrak…) | QByteArray, QString, QDateTime, QJson*, QXml* | **the main work**: parsing logic is pure but written against QtCore |
| `src/widgets/*/*_model.*` | QObject, QVariantList, QTimer | not reused; replaced by Swift view models fed from parser output |
| `src/net/polled_source`, `telnet_client` | QObject, QTimer, QNetwork | not reused; Swift `URLSession` / `NWConnection` |
| `src/platform/settings`, `credential_store` | QSettings; Keychain `.mm` | not reused; see §6 |

### The Qt-free shim

Qt cannot be built for tvOS, so the core must compile without it. Options:

- **A (recommended): a thin `hcp::` compatibility layer** (`core/hcp_compat.hpp`): `hcp::Bytes` (std::string_view), `hcp::DateTime` (int64 ms UTC + ISO-8601 parse/format), and a small JSON reader. The parsers are changed from `QByteArray`/`QJsonDocument` to these types. On the Qt build the same parsers get thin adapters, so Linux/Pi/macOS behaviour and tests do not change.
- **B: vendor nlohmann/json + std::chrono**, rewrite parsers once as pure C++17 and have the Qt models call them. Cleaner end state, larger one-time diff touching every area's parsers.
- **C: re-implement parsers in Swift** (Codable), keeping only astro/SGP4/projection in C++. Least C++ churn, but two parser implementations to keep in sync across 30 products. Not recommended.

Because the parsers belong to other areas, the migration must be incremental and backward compatible: a CMake option `HCP_CORE_WITHOUT_QT` (default OFF) builds `hamclock-core` from the Qt-free sources only; each parser moves into it as it is ported, with the existing Qt tests kept as the oracle. Shared code stays Qt 6.4 compatible; Linux/Pi builds are unaffected. Item 3 of the backlog only needs the SWPC parsers (Kp, flux, X-ray) plus clock/astro, so the first port is small.

## 3. Screens

One page, like the other platforms, using the shared pane registry names so the layout config can be reused.

- **Layout:** a 1920×1080-point canvas (tvOS renders 4K as 2× scale of the same points, so one layout serves both). 90 pt title-safe margins left/right and 60 pt top/bottom. Left ~62%: world map with day/night, DE/DX, overlays. Right: a 2×N grid of cards (space weather, bands, sun/moon, weather, satellites…). Top bar: UTC and local clock, DE/DX callsign and grid.
- **Type:** minimum 29 pt body text (TV viewing distance); numbers 48–96 pt. High-contrast dark theme only.
- **Siri Remote:** the focus engine moves between cards (`focusable`, `@FocusState`). Select on a card opens it full screen (detail view: longer X-ray plot, satellite pass list, 3-day Kp bars). Play/Pause cycles pane category; Menu goes back. Swipe on the map pans between preset views (world / DE-centred / DX-centred). No text entry except in Settings (system keyboard / Continuity keyboard; location by grid or city search).
- **Pane switching:** same categories as the Qt UI (area A); "auto-rotate" option for unattended display, off by default.
- **Burn-in/idle:** keep-awake while shown (`isIdleTimerDisabled`), a slow 1 px/min pixel shift on static chrome, and a dim-after-N-hours option.

## 4. Data refresh and caching

- `URLSession` with `URLCache` and our own `ETag`/`Last-Modified` handling; per-source intervals taken from the same table the Qt app uses (SWPC 1–5 min, USGS 5 min, CelesTrak TLEs daily, etc.), exponential backoff on failure.
- A `Source` actor per document: fetch → hand bytes to the C parser → publish struct plus `fetchedAt`. The UI shows age and flips to Stale/Offline at the thresholds area A defines.
- Background: tvOS apps get no background refresh worth relying on; refresh only while foregrounded and refetch immediately on activation.
- Telnet DX cluster (item for later): `NWConnection`, foreground only; PSKReporter/POTA are plain HTTPS and come first.

## 5. Map

Draw with SwiftUI `Canvas` from the shared equirectangular `projection`/`viewport` (C++), the land polygons from `land.hpp`, the terminator from `solar_position`, and overlay items (quakes, storms, POTA, PSK, beacons) from the same overlay data model used by area G, serialised across the C ABI as plain structs (lat, lon, kind, label, colour token). Overlay items only draw while their source is live, matching the Qt rule.

## 6. Settings, persistence, App Store notes

- tvOS gives apps **no guaranteed local persistent storage**: the system may purge `Caches` and there is no `Documents`. Use `NSUbiquitousKeyValueStore` (iCloud key-value, 1 MB) for settings (callsign, grid/location, units, pane layout, theme), with `UserDefaults` as the local copy that may be wiped. Fetched data lives in `Caches` and is treated as disposable.
- Credentials (QRZ, HamAlert…): Keychain with iCloud sync is available on tvOS; defer lookup-type features until the owner decides they belong on TV.
- First run needs a location. With no keyboard that is awkward: offer (a) type a Maidenhead grid or city via the on-screen keyboard, (b) enter it on iPhone/iPad and sync through iCloud KVS, (c) a companion-code page later. Recommendation: a+b.
- App Store: privacy manifest (no tracking, no collected data), ATS-compliant HTTPS only, attribution screen, age rating 4+. Apple TV requires the app to function with the Siri Remote alone. TestFlight: section 11.

## 7. CI (backlog item 2)

`.github/workflows/tvos.yml`, macOS runner, on PRs touching area I paths and on main: configure the core with the CMake toolchain (`CMAKE_SYSTEM_NAME=tvOS`, `appletvsimulator` sysroot), build the static library, and run the core's unit tests on the macOS host build. Later, an `xcodebuild` Simulator build of the SwiftUI app and a screenshot artifact. No Linux-side effect.

## 8. Effort estimate

| Slice | Estimate |
|---|---|
| CI + tvOS CMake toolchain for core (item 2) | 1–2 runs |
| Qt-free shim + port of SWPC parsers + C ABI | 3–4 runs |
| Minimal SwiftUI app: clock + live space weather + Simulator screenshot (item 3) | 3–4 runs |
| Remaining panes (about 12 of the 30 parsers are sensible on TV) | 10–15 runs |
| Map + overlays | 4–6 runs |
| Settings, iCloud KVS, attribution, polish | 4–5 runs |

Roughly 25–35 three-hourly runs, i.e. about 4–5 days of continuous operation, plus owner time for signing and App Store submission.

## 9. Decisions requested from the owner

1. Shim approach: **A** (compat layer, recommended), B (vendored JSON, full rewrite) or C (parsers in Swift).
2. Panes to include on TV first (proposed: clock, space weather, bands, sun/moon, weather, satellites, quakes/storms, map).
3. Settings entry: grid/city on-screen plus iPhone/iPad sync via iCloud (recommended), or on-screen only.
4. OK to add the `HCP_CORE_WITHOUT_QT` CMake option (default OFF) touching shared CMake minimally.

## 10. App Store record: one record, universal purchase (decided 2026-10-28)

The tvOS app uses the iOS bundle id `com.chrislaw.hamclockpro` and ships as the tvOS platform of the existing **HamClock Pro** record (app id 6822478434). It was `com.chrislaw.hamclockpro.tv` before; App Store Connect only lets a record carry tvOS when the tvOS build has the same bundle id as the iOS one.

| | One record (chosen) | Separate tvOS record |
|---|---|---|
| Purchase | Universal: buy or download once, get iPhone, iPad and Apple TV | Two listings; a paid app would be bought twice |
| Listing | One name, one product page, one set of ratings and reviews; per-platform screenshots, description and version | Its own name, page, ratings, age rating, privacy answers |
| TestFlight | Same testers and groups; tvOS builds appear in their own build list | A second tester setup |
| iCloud key-value store | Same identifier by default, so settings typed on iPhone/iPad reach the TV (section 6) with no extra setup | Needs an explicit shared `ubiquity-kvstore-identifier` entitlement |
| Releases | tvOS versions and builds are numbered and reviewed separately from iOS, so neither blocks the other | Fully independent |
| Reversibility | Hard to split later: moving tvOS out means a new bundle id and record, and TV users would have to install a different app | Can be merged only by re-shipping under the iOS bundle id |
| Privacy label | One App Privacy answer covers all platforms, so the tvOS app must not collect anything the iOS answers don't declare (today neither collects any data) | Separate answers |

The only real cost is that the decision is sticky, and it matches what the product is: the same dashboard on another screen. A separate record would only pay off if the TV app were going to be sold or named differently.

Build numbers don't collide: App Store Connect keeps tvOS builds apart from iOS builds, so the tvOS job uses its own workflow run number.

## 11. TestFlight on the Apple TV

`.github/workflows/tvos.yml` job **Apple TV TestFlight upload** builds the core for `appletvos` (arm64), archives the app with the iPad job's signing route (the imported Apple Distribution certificate plus a temporary `TVOS_APP_STORE` profile that `packaging/apple/asc_profile.py` creates and deletes through the App Store Connect API), exports with method `app-store` and uploads. Like the iPad job it runs only on pushes to main and only while the repository variable `TESTFLIGHT_UPLOADS` is `true`.

App Store validation: `App/Info.plist` sets `ITSAppUsesNonExemptEncryption` to false (HTTPS only), `App/Resources/PrivacyInfo.xcprivacy` declares no tracking, no collected data, and the UserDefaults (CA92.1) and file-timestamp (C617.1) API reasons, and `App/Resources/Assets.xcassets` holds the layered tvOS app icon (400x240 @1x/@2x, 1280x768 App Store) and both top shelf images. `python3 apple/tvos/make_tv_icons.py` regenerates them.

### One-time owner steps

1. **Add the tvOS platform to the record.** App Store Connect → Apps → HamClock Pro → in the left sidebar, Add Platform (the **+** next to the iOS/macOS entries) → tvOS. A tvOS version page appears (version 0.1.0 is fine for TestFlight). Uploads fail until this exists.
2. **No new secrets.** The job reuses `ASC_KEY_P8_BASE64`, `ASC_KEY_ID`, `ASC_ISSUER_ID`, `IOS_DIST_CERT_P12_BASE64`, `IOS_DIST_CERT_PASSWORD` and the `APPLE_TEAM_ID` variable. The Apple Distribution certificate signs tvOS apps too, and the bundle id `com.chrislaw.hamclockpro` already exists in the developer account. Nothing to type into `gh secret set`.
3. **Upload a build.** Same switch as the iPad: in a terminal, `gh variable set TESTFLIGHT_UPLOADS --body true`, then re-run the latest main run of the **tvOS** workflow (Actions → tvOS → latest main run → Re-run jobs → Re-run all jobs, or `gh run rerun <run-id>`), then `gh variable set TESTFLIGHT_UPLOADS --body false` once the job is green. A run re-run keeps its run number, so re-run a newer main run (or push to main) for each further upload.
4. **Let the build through TestFlight.** It takes 5–30 minutes to process. App Store Connect → HamClock Pro → TestFlight → tvOS builds: the build should show no "Missing Compliance" warning (the plist answers it). Add it to the internal group you already use for the iPad builds if that group doesn't have automatic distribution on.
5. **Later, optional: iCloud settings sync.** The TV keeps its settings locally until the App ID has the iCloud capability (Certificates, Identifiers & Profiles → Identifiers → com.chrislaw.hamclockpro → iCloud, key-value storage) and the app gets the `com.apple.developer.ubiquity-kvstore-identifier` entitlement. That is a separate change.

### Installing on the Apple TV

1. On the Apple TV, make sure Settings → Users and Accounts → the default user is signed in with the Apple ID that is an internal tester on the HamClock Pro record (the developer account's Apple ID is).
2. Open the App Store on the Apple TV, search for **TestFlight** and install it.
3. Open TestFlight. HamClock Pro is listed once a tvOS build has finished processing; choose it and select **Install**. Later builds show **Update**.
4. HamClock Pro then appears on the Home Screen like any app. TestFlight builds expire after 90 days; upload a new one to keep it installed.

If HamClock Pro isn't in TestFlight on the TV, check that the build finished processing, that the TV is signed in as the tester's Apple ID, and that the tester's group includes the tvOS build.
