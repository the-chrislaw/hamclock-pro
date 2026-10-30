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

## 12. Parity rebuild (2026-10-29)

The first TestFlight build used system glass cards, a bare UTC clock, inline locator fields and a "Panes…" on/off sheet. It did not look or work like the main app. The SwiftUI front end was rebuilt to mirror the Qt layout and behaviour, still over the Qt-free core.

### Layout and look

- Same structure as `Main.qml` on a wide screen: a "HamClock Pro" header with a Settings button, then the station card on the left and, on the right, a row of three pane slots over the world map. With **Settings > Map > No map** there are two rows of panes instead.
- Colours come from `Theme.qml` (`Theme.swift`): canvas, surface, ink, secondary, divider, accent (five styles), focus, good/warn/bad. Dark is the default; Light is offered as in the main app. Map colours come from `src/map/map_renderer.cpp` (sea, land, coast, route).
- Card chrome follows `Card.qml`: title with ▾ when Select changes the widget, a Live/Stale/Loading/Offline/Set up badge (dot and word), the body, then the source and the data age ("just now", "5 min ago"). The body never sizes the card; content that does not fit is clipped, not spilled.
- The station card follows `StationCard.qml`: callsign banner (colour, background and custom title from Settings), big clock (UTC or local first, 12/24 h, date order), the other clock, time zone, DE and DX grid, coordinates, local time at DX, sunrise/sunset countdowns, short or long path.
- Type: 29 pt minimum for body text; the band table uses 26 pt so both halves fit in a pane.

### Siri Remote

| Where the focus is | Select | Hold Select | Play/Pause | Menu |
|---|---|---|---|---|
| Station card | swap UTC / local clock (opens setup when no station) | Settings | cycle banner (call, ON AIR, 73 DE, custom title) | leave the app |
| Pane | pane chooser | pane options | pane options | leave the app |
| Rotating pane | pane chooser | pane options | pane options | leave the app; **left/right step the widget** (back and forward, wrapping), up/down move the focus |
| Map | map menu | map menu | map menu | leave the app |
| Settings button | Settings | – | – | leave the app |
| Any overlay or Settings | choose | – | – | close it |

- The **pane chooser** has the main app's category tabs (All, Space Wx, DX & Contest, Weather, Sky & Space; Station has no TV widgets so its tab is hidden) and lists only widgets with a TV data source. Empty pane is at the bottom.
- **Pane options** follow `PaneOptions.qml`: Change widget, Next/Previous widget (registry order, "none" between last and first), Rotate (off, 5, 10, 30, 60 s; the rotation is never saved as the pane's choice), Empty pane, Reset all panes.
- The **map menu** sets the centre (World, DE, DX), zoom 1–4×, short or long path, and the overlays: day/night, lat/lon grid, borders, DE–DX path, Sun, earthquakes, tropical storms, satellites, POTA/SOTA/WWFF, fires, launch pads, planets, aurora, D-RAP. Overlays draw only while their source is Live or Stale, never from Offline data.
- **Rotating panes** (owner request): while a rotating pane has the focus it shows "◀ ▶ change · rotates every N s". Left/right (click or swipe) step it like `PaneSlot.step` in the main app: the widget shown becomes the pane's choice and the rotation restarts from there, so it does not change again for a full interval. Up/down still move the focus (the header is a full-width focus section, so Up from any pane reaches Settings). Nothing else on the TV advances on its own.
- **Menu at the root** (no overlay, no Settings) has no handler in the app, so tvOS goes back to the Home screen, also while dimmed and straight after closing a menu or Settings. With Settings or an overlay open, Menu closes it, even before the focus has moved into it. `testMenuAtRootExitsToHomeScreen` checks each case.
- No text fields on the dashboard and no sheets: overlays are centred panels over the dimmed dashboard; Settings is a full-screen layer.

### First run and Settings

With no station the app opens **Set up HamClock Pro**: callsign, home by place search (OpenStreetMap Nominatim, sent only when Find is pressed, as in the main app) or Maidenhead grid, optional DX grid, and the time zone (suggested from Open-Meteo for the coordinates, falling back to the main app's reference-city table; changeable from a region list). "Later" leaves it unset; the station card then says so and Select reopens it.

Settings has the main app's Station, Appearance, Distance units, Clock format (date order, 12/24 h, UTC or local big clock), Callsign banner (colour, background, custom title), Accent colour and Map sections, plus Panes (reset), Screen (dim after 2/4/8 h without remote input) and Data sources. Display choices apply and save at once; the station saves with "Save station".

### Persistence and iCloud

Settings are flat `hcp.*` keys with plain property-list values (`AppSettings.swift`, `AppSettings.Key`): `hcp.station.*` (callsign, DE/DX latitude and longitude, zones, place name), `hcp.display.*`, `hcp.panes.slots` (12 widget ids), `hcp.panes.rotations`, `hcp.map.size`, `hcp.map.layers`, `hcp.tv.dimAfterHours`, `hcp.passes.filter`. Each write goes to `UserDefaults` and to `NSUbiquitousKeyValueStore`; external KVS changes are adopted. Builds before this one stored only `station.locator`; it is migrated to DE coordinates on first launch.

**Next step for iCloud sync (not done here):** the app does not carry the `com.apple.developer.ubiquity-kvstore-identifier` entitlement yet, so the KVS writes stay on the device. Signing only happens in the TestFlight job on main, so a PR cannot show that the entitlement signs with the API-created tvOS profile. To turn it on: add `App/HamClockTV.entitlements` with `com.apple.developer.ubiquity-kvstore-identifier` = `$(TeamIdentifierPrefix)com.chrislaw.hamclockpro`, set `CODE_SIGN_ENTITLEMENTS` in `project.yml`, and check the next TestFlight archive. The iPhone/iPad app (Qt, `QSettings`) would also have to mirror the same `hcp.*` keys to KVS before settings typed on the phone reach the TV.

### Widgets on TV

Each TV widget shows what the Qt card shows. Parsers are Qt-free ports in `apple/tvos/Core` with regression tests against the Qt widget fixtures.

| Widget | Source | Core |
|---|---|---|
| Space weather (Kp, SFI, X-ray, 6 h X-ray plot) | NOAA SWPC | `swpc` |
| Solar wind (speed, Bz, sparklines) | NOAA SWPC RTSW | `solarwind` |
| Band conditions (day/night, 80–10 m) | SFI and Kp (band_outlook heuristic, not VOACAP) | `bands` |
| HF path reliability (DE → DX, 80–10 m × next 24 UTC hours, best band now; SSB 100 W 6 dBi) | SFI and Kp (in-house path model, approximate, not VOACAP) | `pathrel` |
| NOAA scales (R/S/G, 24 h max, today's probabilities) | NOAA SWPC | `scales` |
| Aurora (hemisphere peaks; map overlay) | NOAA SWPC OVATION | `aurora` |
| D-RAP (highest affected frequency; map overlay) | NOAA SWPC | `drap` |
| Sun (SDO) | NASA SDO AIA 193 Å image | – |
| Sun & Moon (DE/DX rise and set, Moon phase) | computed on device | `astro_sun`, `astro_moon` |
| Satellites (above the horizon at DE; map overlay) | CelesTrak amateur TLEs, AMSAT `nasabare.txt` when CelesTrak fails; SGP4 on device | `astro_satellite` |
| Passes (next 48 h at DE: when, peak elevation, rise → set; All / Visible / Alerts filter in pane options; Settings > Satellite pass alerts also puts a banner on the dashboard before and during passes of the chosen birds, from the shared amateur-satellites.json catalog) | the Satellites elements, SGP4 on device | `passes`, `satalert` |
| Planets (up/below, next rise/set; map overlay) | computed on device | `astro_planets` |
| Next eclipse (type, date, visibility from DE) | computed on device | `astro_eclipse` |
| Launches (countdown, provider, ham hint; pads on map) | Launch Library 2 | `launches` |
| Weather (temperature, wind, humidity, pressure trend) | Open-Meteo | `weather` |
| Earthquakes (strongest, latest; map overlay) | USGS M2.5+ day | `quakes` |
| Tropical storms (map overlay) | NOAA NHC | `storms` |
| Active fires (US, 24 h; map overlay) | NASA FIRMS VIIRS | `fires` |
| On the air (POTA/SOTA/WWFF spots; map overlay) | POTA, SOTA, WWFF | `ota` |

Not on TV yet, and why: DX cluster, HamAlert and Meshtastic (telnet/LAN sessions with a login; a foreground-only `NWConnection` client is the next piece of work), PSKReporter (feed not ported), Space weather impact (parser not ported yet; Dst index, Space alerts, Solar trend, HamQSL and HF path reliability are ported), Beacons and Watch list (depend on the cluster), hams.at and Callsign lookup (personal API keys or accounts), Radio, Rotator, GPS, ADS-B, Host sensors and the ADIF log (station hardware and local files). A pane saved with one of these on another device says "Not available on Apple TV yet".

### Verification

- Core: `cmake -S cmake/TvOSCore -B build-host && cmake --build build-host && ctest --test-dir build-host` (29 tests; `tvos.yml` also runs them inside the tvOS Simulator).
- Threading: feed bodies are parsed (and written to the Caches copy) on a background task, never on the main actor, and satellite positions are computed every 5 s off the main actor with one batched core call. Parse times show up in Instruments as `parse` signpost intervals (subsystem `com.chrislaw.hamclockpro`).
- App: `apple/tvos/UITests/RemoteWalkthroughTests.swift` drives the Siri Remote (`XCUIRemote`) through first-run setup, the pane chooser, pane options (hold Select and Play/Pause), the map menu and a settings change, then relaunches and checks that the station, the pane choice and the units were kept. Locally: `xcodebuild test -project apple/tvos/HamClockTV.xcodeproj -scheme HamClockTV -destination 'platform=tvOS Simulator,name=Apple TV 4K (3rd generation) (at 1080p)'`; set `TEST_RUNNER_HCP_SHOT_DIR=<dir>` to also write the screenshots. `tvos.yml` runs the same test and uploads the screenshots as the `tvos-simulator-screenshots` artifact.
- Screenshots in `docs/images/tvos/` are tvOS Simulator captures with the live data of the moment they were taken, not photos of a TV. Nothing here has been checked on an Apple TV yet.
