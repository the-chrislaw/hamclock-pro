# Widgets: contract and backlog

Owner decision (Oct 13): every widget works with live data, in a modern Qt Quick UI that scales from a 7-inch touchscreen to a 4K TV. Work runs in parallel, one agent per **area** below; each area owns its folders so agents do not collide.

## The widget contract

A widget is one folder `src/widgets/<id>/` (id: lowercase `[a-z0-9_]`):

```
src/widgets/<id>/
  CMakeLists.txt        hamclock_add_widget(<id> SOURCES … QML … TESTS … TEST_DATA_DIR tests/fixtures)
  <name>_model.hpp/.cpp subclass of hamclock::WidgetModel (src/widgets/widget_model.hpp)
  <Name>Card.qml        root is `Card { }` from `import HamClock`; reads its model as `widget`
  tests/*.cpp           offline tests; fixtures are trimmed real responses with capture date
  tests/fixtures/*      named after the URL's file name, so `--fixtures` can serve them
```

- **Register** with `void hamclock::register_widget_<id>(WidgetRegistry&, const WidgetContext&)`: create the model, `registry.add({id, title, QUrl("qrc:/widgets/<id>/<Card>.qml"), area, order}, model)`, start it with `QTimer::singleShot(0, …)`. `area` is `"strip"` (card row above the map), `"side"` or `"wide"` (full-width panel under the map). Adding a widget never edits root CMake or `Main.qml`.
- **Data:** use `PolledSource` (`src/net/polled_source.hpp`) with `context.transport` for every HTTP fetch: https only, honest User-Agent, backoff, disk cache. Parse in pure functions tested on fixtures. Long-lived connections (DX cluster telnet) get their own class with the same injected-transport idea; no test may touch the internet.
- **No internet in the agent sandbox:** outside websites return 403 there. Build fixtures from the real samples in `tests/fixtures/sources/` (see its README). If a source you need has no sample, do not invent one: open or update the issue `Owner input needed: data samples` listing the exact URLs, and work on another item meanwhile.
- **Portable code:** the same sources build for Raspberry Pi, macOS and iPadOS. No QtWidgets in new code; Linux/Pi-only and Apple-only code lives behind small interfaces in `src/platform/`; no shelling out, no hard-coded paths (use `QStandardPaths`); network code must tolerate the app being suspended (iPad) and reconnect on resume.
- **One page, no scrolling (owner requirement, Oct 13):** like the original HamClock, the whole dashboard fits on one screen at every size from 800×480 to 4K, with no scrolling. The layout is a fixed grid of **slots** (station/clock column, a row of panes, the map, optional side panes). Each slot shows one widget; the user taps or selects a pane's title to choose which widget it shows (the original's pane menu), with optional auto-rotation. Every widget card must therefore render fully inside its slot at the smallest size: provide a compact layout (key numbers first, details hidden or shortened), never overflow, never require scrolling. Lists (spots) show as many rows as fit.
- **Logs:** each area appends to its own `docs/agent-logs/<area>.md` (ui, space, spots, astro, wx, apple, map, hazards, qa), never to a shared file, so parallel PRs do not conflict. `docs/agent-log.md` is the historical log up to Oct 13 and is read-only. In the Status table, edit only your own area's rows.
- **States:** call `setState()` / `trackSource()` so the card shows `live`, `stale` (with age), `loading`, `offline` or `unconfigured`. Never show an invented or stale-as-live value; missing values render as "—" (`Metric { available: false }`).
- **Settings and accounts:** read from `context.settings` (`AppSettings`). New settings go in `AppSettings` and `SettingsPage.qml` (UI area owns those files; other areas request additions in their PR description or add a small, separate field-only change). Passwords are C++-only, never QML properties.
- **UI:** build from `Card`, `Metric`, `StatusBadge`, `AppButton`, `FormField` and `Theme` tokens. Touch targets ≥ `Theme.touch`; everything reachable by keyboard (TV remote); text scales with `Theme.scale`. Cards sit in clipped dashboard slots: read `Card.compact` (true in a small slot) and show key numbers only when it is set. The shell sets `Card.chooser` so the title opens the pane chooser; any widget id can be placed in any slot, whatever its registered `area`.
- **Screenshots:** `hamclock-pro --config-dir DIR --fixtures src/widgets/<id>/tests/fixtures --theme dark --size 1024x600 --screenshot out.png` renders deterministically without the network; add `--pane N=<widget id>` (N = 1..3, repeatable) to choose what each pane shows, since every widget can be screenshotted this way (CI renders each one at 800×480). Check 800×480, 1024×600, 1280×800, 1920×1080, light and dark.
- **Sources:** only those in the roadmap's Data source policy or listed below; record terms and polling in the PR before first use.

## Areas and backlog

Each area (A–H): one owner agent, branches `agent/<area>-<slug>`, files listed are exclusive to that area.

### A. UI shell and design (owns `src/ui/`, `cmake/HamClockApp.cmake`, `docs/WIDGETS.md` contract section)
0. 🟡 **Top priority: one-page slot layout** (done: no scrolling, three pane slots, chooser, choices saved; map now fills its item without the widget's inner margin; done: dense small-screen shell (no header, tight spacing, settings button in the station card) and tap-anywhere/swipe pane switching; done: long-press pane options (change, step, empty, reset); done: per-slot rotation (off/5/10/30/60 s through all widgets, set from the long-press menu, saved); pending: custom rotation sets, side panes, `compact` layouts in each widget card). Remove page scrolling. Build the fixed slot grid (station column + pane row + map, scaling from 800×480 to 4K), a pane chooser (tap pane title → list of widgets; keyboard/remote accessible), per-slot choice saved in settings, optional rotation interval per slot. Default arrangement at 1024×600: station column left, three panes on top (Space weather, Solar wind, DX cluster), map filling the rest. Verify by screenshots at 800×480, 1024×600, 1280×800, 1920×1080 that nothing is clipped and nothing scrolls. Real-screen issues to fix: docs/hardware-validation.md.
0c. **Owner priority (Oct 13): station column parity with the original's DE/DX panels.** Do this right after A0, before 0b. Owner found the left column missing much of the original. Each item compact enough for the 7-inch 1024×600 column; verify by screenshot:
   - Callsign banner: call in large type with fg/bg colours (rainbow option), tap to cycle call / custom title / ON AIR (A13).
   - DE box: local time and date in selectable formats (digital 12/24 h, analog, calendar, annotated analog), DE sunrise and sunset shown as "in 3h 12m" / "ago" or "at" times (data from area D's astro code).
   - DX box: DX local time with automatic time zone from DX coordinates (manual override), DX sunrise/sunset, prefix → country name, short-path and long-path distance/bearing with a tap toggle, true or magnetic bearing (WMM), units Imperial/Metric.
   - Tap targets as in the original: tap DE or DX to set a new location (grid, lat/lng or search, reusing 0b), tap the clock for Big Clock / stopwatch / countdown, tap UTC to toggle UTC/local.
   - Auxiliary time line (date, day of year, JD/MJD, sidereal, UNIX) selectable (A14 subset).
0b. **Settings: location lookup.** "Find" button: search an address or place via OpenStreetMap Nominatim (honest User-Agent, max 1 request/second, only on button press; sample in tests/fixtures/sources), fills latitude, longitude, grid square and suggests the time zone; Maidenhead grid entry as an alternative; on macOS/iPad also "Use my location" via the OS location service (with area F).
1. Match `design/dashboard.html` closely: header, sidebar, card spacing, typography, map toolbar; smooth touch scrolling; pressed/focus states.
2. TV mode: overscan inset setting, remote/keyboard focus order across cards, focus ring, large type.
3. Settings: locator entry (Maidenhead) as alternative to lat/lon, time-zone picker list, map click to set DX.
4. Map: full-card map, pan-free zoom presets, grayline band styling, DE/DX labels; then port the map drawing from QWidget to a pure QPainter renderer.
5. Port Big Clock, Stopwatch, Countdown, DE time settings dialogs into QML pages; then retire the Qt Widgets programs (`hamclock-pro-dashboard`, `-editor`, `-map-preview`).
6. Pane rotation (F26): user chooses which widgets show in which slot; layout saved.
7. Kiosk: systemd unit + `--fullscreen`, eglfs instructions; on-screen keyboard decision.

### A. (audit additions, Oct 13; see docs/FEATURE-AUDIT.md)
8. Pane system parity (refines A0): 4 panes plus optional PANE_0 overlay on the DE/DX area; each pane has its own set of widgets, rotation 5/10/30/60 s, forward/back stepping, category picker (Space Wx, DX & Contest, Weather, Sky & Space, Station & Sensors, Other); saved layout.
9. Map styles and projections: Mercator with centre longitude, Azimuthal (DE-centred), Azimuthal-one, Robinson; zoom 1–4× with recenter/reset; tap popup Set DE / Set DX; selectable base styles that rotate every 20/60/90/120 s; colour-scale legends.
10. Map overlays: Tropics, lat/lng, Maidenhead (fields and 4-char), azimuth rings, CQ and ITU zones, cities (tap snaps to city), country borders and US states, DE antipode, sublunar point, long-path drawing, gray display mode.
11. Hover/long-press info box: lat/lng, grid, prefix/country, distance/bearing, local mean time, weather, details of the object under the pointer.
12. DE/DX panels: DE formats (All info, Simple analog, Calendar, Annotated analog, Digital 12/24 h), DX time zone (auto/manual), sunrise/set "in/ago" vs "at", SP/LP toggle, true or magnetic bearing (WMM), units Imperial/Metric/British.
13. Callsign banner: call / rotating titles / ON AIR text, fg/bg colours, rainbow option.
14. Clocks: auxiliary time (Date, DOY, JD, MJD, Sidereal, Solar, UNIX), change-time ("time travel") menu, date order and week start, daily and one-time alarms (sound, optional GPIO), Big Clock option parity (LST, +digital, numbers, coloured hands, DE WX, Space WX, satellite).
15. Kiosk management: brightness min/max, light-sensor auto-dim where available, daily on/off schedule per weekday, idle blanking, screen lock, restart/exit/reboot/shutdown menu, per-action passwords via the credential store.
16. Saved configurations (save/restore/update/delete/rename) and demo mode.
17. Colour editor: SP/LP path, satellite 1/2, grid, rotator, per-band colours, borders, states; dashed/thick.
18. Remote API (F28, after a security review): read-only and read-write endpoints matching the original `get_*.txt`/`set_*` names where practical, remote screenshot, mobile status page, live view.

### B. Space weather and solar (owns `src/widgets/spaceweather/`, `solar*`, `bands*`)
1. ✅ Kp, solar flux, X-ray class (NOAA SWPC) — the reference widget.
2. ✅ Solar wind speed/density and Bz (SWPC `json/rtsw/*_1m.json`; the old `products/solar-wind/*` URLs are gone), with sparkline history.
3. ✅ Sunspot number and 30-day solar flux trend chart; Kp 3-day bar chart (`solartrend`: SWPC `f107_cm_flux.json`, `observed-solar-cycle-indices.json`, `noaa-planetary-k-index-forecast.json`; polled every 6 h / 12 h / 30 min).
4. ✅ Band conditions (F07): day/night HF band outlook computed from SFI/Kp/A (model documented in `bands/band_outlook.hpp`; `bands` widget). Optional KC2G MUF map not done.
5. ✅ Solar imagery (F09): NASA SDO latest images (AIA 193/304, HMI), cached, tap to cycle (`solarimage`).
6. ✅ NOAA alerts/watches/warnings feed for storms and radio blackouts (`solaralerts`: SWPC `products/alerts.json`, newest message per product, cancellations, expired warnings and old alerts dropped).

**Audit additions (B):**
7. DRAP pane and DRAP map style.
8. Aurora pane, Aurora map style, and automatic switch to the Aurora/DRAP map when activity is high.
9. DST (Disturbance) pane.
10. NOAA R/S/G scales pane.
11. ✅ X-ray 24 h history plot (`spaceweather`: SWPC `xrays-1-day.json`, 10-minute peaks on a log A1–X10 axis with class lines; polled every 15 min, hidden when out of date; compact card shows a thin version).
12. Space-weather stats ranked by impact.
13. VOACAP-style DE→DX reliability matrix (power, mode, takeoff angle, SP/LP, UTC/DE timeline, antenna models) plus per-band TOA/REL and MUF maps; needs a source agreement or an offline open-source model.
14. HamQSL (N0NBH) HF and VHF condition panes (check terms first).
15. Tropo map style (check source terms first).

### C. Spots (owns `src/widgets/dxcluster/`, `pskreporter/`, `ota/`, `src/net/telnet*`)
1. DX cluster (F13): telnet client (DXSpider/AR-Cluster/CC-Cluster), callsign login, reconnect/backoff, spot parser, spot list card, spots on the map, band/mode filters.
2. PSKReporter (F14): reception reports for your callsign (respect its query-rate limits), map overlay.
3. POTA and SOTA activator spots (public APIs).
4. Watch lists (F16): highlight callsigns/prefixes/DXCC entities; alert badge.
5. Callsign prefix → DXCC entity/country lookup (F12) from a bundled, licensed prefix table.

**Audit additions (C):**
6. WSPR and RBN in Live Spots: of DE / by DE, by call or grid, max distance or count, band statistics, map paths.
7. More On-The-Air programmes (WWFF, IOTA, GMA, LLOTA, WWTOTA, TOWERS, WWBOTA) with age/sort/mode/band filters.
8. HamAlert pane. 9. Nearby APRS pane (APRS-IS, radius filter, receive-only). 10. Meshtastic pane and map links. 11. Band-activity heatmap (band × continent).
12. UDP spot input (WSJT-X, N1MM/DXLog, Log4OM), multicast, "UDP sets DX", "by me"/"all".
13. Cluster login with SSID and native commands after login.
14. Spot labels None/Prefix/Call/Dot, minimum label distance, per-source map on/off; map overlays for ONTA, ADIF and HamAlert.
15. ADIF log pane with file watching and not-in-log watch keywords (NADXCC/NAPref/NABand/NAGrid).
16. IOTA reference names and xOTA markers parsed from comments.

### D. Astronomy, satellites, beacons (owns `src/widgets/moon/`, `satellites/`, `beacons/`, `src/domain/astro*`)
1. ✅ Sun rise/set at DE and DX, day length, grayline times (offline). Polar day/night handled.
2. ✅ Moon phase, rise/set, elevation/azimuth at DE (F04) (offline).
3. 🟡 (SGP4, look angles, pass search, TLE download and a next-passes pane done; map track and the user's choice of up to two satellites next) Satellites (F05): CelesTrak TLEs (cached daily), SGP4 propagation (vendored library with licence recorded), next passes at DE, live ground track on map, up to two tracked satellites.
4. ✅ NCDXF/IARU beacons (F19): current transmitting beacon per band, the five on-air beacons marked on the map (offline schedule).
5. EME planning (F06): Moon position at DE and DX, mutual window.

**Audit additions (D):**
6. Satellite pass sky plot in the DX panel, co-visibility planner, group schedule, frequency table and status links.
7. Satellite rise/set alert; maximum TLE age; user-supplied TLE.
8. hams.at upcoming activations pane. 9. Next solar eclipse pane. 10. Planets on the map. 11. Yearly sunrise/sunset ("grayline") chart.
12. Moon pane image with phase, linked to the EME tool. 13. Launches pane. 14. Balloons pane.

### E. Weather, lookup, events, radio (owns `src/widgets/weather/`, `lookup/`, `events/`, `radio/`)
1. ✅ DE/DX weather (F10) via Open-Meteo (no key): temperature, wind, conditions, at both ends.
2. 🟡 Callsign lookup (QRZ XML or HamQTH; card + session handling done, spotted-call wiring pending) using the account in Settings: name, grid, country for DX and spotted calls; session key handling, no password logging.
3. Contests (F17) and DXpeditions calendars; on-the-air events (F18) — check each feed's terms first.
4. Radio control (F20): read frequency/mode from rigctld/flrig on localhost or LAN (read-only first), show on header; rotator later. Safety review before any transmit-affecting command.
5. GPS (F21) via gpsd for DE location; CPU temperature/sensors (F22).

**Audit additions (E):**
6. Weather parity: humidity, pressure and trend, clouds, city, DX time zone; brief weather popup on new DE/DX.
7. Weather and clouds map styles; Windy and WEFAX links.
8. Hazard panes and map overlays: quakes (USGS), tropical storms (NHC/JTWC), NWS marine and fire-weather warnings, lightning with range rings, active fires (NASA FIRMS); check each feed's terms first.
9. Radio: PTT-driven ON AIR, then (after a safety review) tune to a tapped spot; flrig and rigctld.
10. Rotator pane (rotctld) that can track the selected satellite or DX.
11. BME280 environment panes (temperature, pressure, humidity, dew point; corrections); CPU temperature, Wi-Fi RSSI and disk-use history.
12. NTP choice; gpsd/NMEA for time as well as location; IP geolocation.
13. NetLogger active nets pane.
14. GPIO outputs/inputs (alarm, countdown lights, ON AIR, satellite alarm) behind a `src/platform` interface, Linux/Pi only.
15. PiAware ADS-B link.

### F. Apple platforms: macOS and iPad (owns `src/platform/apple*`, `src/platform/credential_store*`, `cmake/Apple*.cmake`, `.github/workflows/apple.yml`, `packaging/apple/`)
Owner decision (Oct 13): ship a signed macOS app and an iPad app after the Pi 1.0 work is under way. The agent sandbox cannot run Xcode, so every change is verified on GitHub's macOS runners via `apple.yml`.
1. ✅ `apple.yml` CI: build and ctest on macOS (Homebrew Qt), and build the app for the iOS Simulator (Qt for iOS via `aqtinstall`, pinned version); upload the macOS app and a Simulator screenshot as artifacts.
2. ✅ (store + tests; AppSettings wiring pending area A) Credential store: `src/platform/credential_store` with macOS/iOS Keychain and a 0600-file fallback elsewhere; AppSettings uses it (coordinate with area A in the PR).
3. 🟡 (Info.plist, category, version, app icon, unsigned dmg done; signing/notarization steps wired in apple.yml and skipped until secrets exist; universal build pending) macOS app polish: app icon, Info.plist (name, version, category), universal (arm64 + x86_64) build, `.dmg`; signing and notarization steps wired but skipped until the owner provides an Apple Developer ID (open "Owner input needed: Apple Developer account").
4. iPad: iOS CMake target, safe-area and orientation handling, touch-first layout check at iPad sizes (1180×820, 1366×1024), app lifecycle (pause polling and the DX cluster connection in background, resume cleanly), no QtWidgets dependency (requires area A's QPainter map renderer, backlog A4).
5. App Store readiness: privacy manifest, no private APIs, attribution screen for data sources (NOAA, NASA, Natural Earth, Open-Meteo…), TestFlight upload once the owner's account exists.

**Audit additions (F):**
6. Mark Pi-only hardware items (GPIO, I2C, light sensor, kiosk power menu) as unavailable on macOS/iPad; check any remote-API listener against App Store rules.

### G. Map and overlays (owns `src/map/`, `src/ui/map*`, `src/ui/qml/Map*`, `src/qt/world_map_widget*`, `src/widgets/mapoverlay*`) — split from A on Oct 13
Takes over map work from area A (A4, A9, A10, and the map part of A0a) so layout and map proceed in parallel.
**Owner priority (Oct 13): the map is the biggest gap against the original. Spots on the map, projections + tap, grids/zones and propagation styles are all wanted, in parallel.** Order below is chosen so other areas can start drawing on the map as early as possible.
1. Port the map from the QWidget renderer to a pure QPainter/Qt Quick renderer (unblocks the iPad build); map fills its card (crop polar latitudes rather than letterbox). Keep the PR small; land it before anything else.
2. ✅ (markers, labels, min distance, DE path; tap/legend later) Overlay API (moved up): a small interface other areas use to draw their objects (spots, satellites, APRS, hazards, beacons) with labels None/Prefix/Call/Dot and a minimum label distance, plus great-circle path drawing from DE. Document it in `docs/map-overlays.md` with one worked example. Areas B, C, D, E and H start their map items as soon as this merges.
3. ✅ (registry, rotation, auto space-weather pick, legend; settings UI comes with the first image style) Map styles framework (moved up): selectable base styles with rotation 20/60/90/120 s and automatic space-weather style, and a colour-scale legend. Individual image styles (DRAP, MUF, aurora, clouds…) come from the owning areas via this framework.
4. Projections: Mercator with centre longitude, Azimuthal (DE-centred), Azimuthal-one, Robinson; zoom 1–4×, recenter/reset; tap popup Set DE / Set DX.
5. Grids and zones: Tropics, lat/lng grid, Maidenhead fields/squares, azimuth rings, CQ and ITU zones, borders and US states, cities (licensed data), DE antipode, sublunar point, long-path drawing, gray mode.

Map items owned by other areas (each starts once G2/G3 merge, using only the overlay/styles API; G reviews nothing, the API is the contract):
- C: DX cluster, PSKReporter/RBN/WSPR and POTA/SOTA spots on the map with paths from DE.
- D: satellite path and footprint, NCDXF beacon markers.
- B: DRAP and Aurora map styles; MUF-RT style if a terms-checked source exists (KC2G candidate).
- E: Clouds and weather styles (direct, terms-checked sources only), and a weather popup when DE/DX is set by tapping the map (as in the original).
- H: storm positions with forecast tracks (NHC), quake markers sized by magnitude, and other hazard markers; the hazard panes stay as the detailed list view.

### H. Hazards and events (owns `src/widgets/quakes*`, `storms*`, `lightning*`, `fires*`, `marine*`, `firewx*`, `nets*`) — split from E on Oct 13
Takes over E8 and E13. Each feed: check terms first and record them in the roadmap's Data source policy.
1. Earthquakes (USGS feeds): pane + map overlay.
2. Tropical storms (NHC/JTWC): pane + track overlay.
3. NWS marine warnings and fire-weather (Red Flag) warnings: panes.
4. Lightning with range rings from DE, active fires (NASA FIRMS): overlays/badges.
5. NetLogger active nets pane.

### QA. Quality reviewer (no feature ownership; writes only `docs/agent-logs/qa.md`, `docs/hardware-validation.md` notes and GitHub issues)
Runs every 4 hours: reviews PRs merged since its last run, builds main, renders screenshots at 800×480 and 1024×600 (light/dark) with all widget fixtures, compares behaviour against the original (docs/FEATURE-AUDIT.md) and the contract (one page, no clipping, compact panes, honest states, portability), and opens a focused GitHub issue per problem labelled `qa` with the owning area in the title (e.g. "[B] Space weather X-ray clipped at 1024x600"). Agents treat open `qa` issues in their area as top of their backlog. Never edits feature code.

## Settings and accounts parity (checked against the original's setup.cpp, Oct 13)

Source: OpenHamClock `ESPHamClock/setup.cpp` (the fork current users run; some items postdate 4.22). Every setting below needs a place in Settings (area A owns the page; each area adds its own fields with minimal, separate changes) and works offline-safe when empty.

| Original setting | Purpose | Owner | Status |
|---|---|---|---|
| Callsign; DE lat/long (decimal or DD:MM:SS, N/S/E/W); grid square (auto from lat/long) | Station | A | Partial: decimal only; add DMS and grid entry (see A0b) |
| DX cluster host, port, **login with SSID (1–99)**, watch list, native commands sent after login, UDP/multicast option (e.g. WSJT-X) | Spots | C | Partial: host/port, login SSID (1–99) and receive-only post-login commands done; watch list and UDP/multicast missing |
| **HamAlert.org** telnet login + password | Personal spot alerts | C | Missing |
| **APRS-IS** server (default rotate.aprs2.net), port 14580, search radius; receive-only login with callsign | Nearby APRS stations on the map | C | Missing |
| **hams.at API key** | Personalised satellite pass info | D | Missing |
| gpsd host **or** serial NMEA device | DE location/time from GPS | E | Partial (gpsd planned) |
| NTP server | Time source | E | Missing |
| rigctld, rotctld, flrig host/port | Radio and rotator | E | Partial (read-only radio planned) |
| PiAware ADS-B receiver host | Aircraft on the map | E | Missing |
| ADIF file path + watch list; On The Air watch list | Log and activity highlighting | C/E | Partial |
| Meshtastic node IDs (neighbour links) | Mesh map overlay | C | Missing |
| BME280 sensors (I2C device, temp/pressure corrections) | Local environment | E | Partial (sensors planned) |
| Brightness dim/bright %, display schedule | Display | A | Planned (F27) |
| Mercator map centre longitude; colour choices | Map/appearance | A | Missing |
| Local passwords guarding restart/exit/unlock/new DE-DX/configurations | Kiosk protection | A | Missing |
| QRZ/HamQTH lookup | *Not in original*: our addition | E | Planned |

Credentials (HamAlert password, hams.at key, lookup password) are C++-only and stored by the credential store (owner-only file, Keychain on Apple), never shown back or logged.

## Status

| Widget | Area | State |
|---|---|---|
| Station + clocks + DE local time | A | Working (QML); dashboard page scrolls (touch flick) when widgets exceed the screen, map keeps a full 2:1 size |
| World map, day/night, path | G | Working (QML, pure QPainter renderer, no QtWidgets; styles registry with legend (no image styles published yet); fills its card, crops polar rows to at least ±70°; the legacy QWidget map remains only for the old dashboard) |
| Space weather: Kp, SFI, X-ray | B | **Live** |
| Solar wind speed/density, IMF Bz/Bt with 1 h sparklines (SWPC RTSW) | B | **Live** |
| Solar trend: sunspot number, 30-day flux trend, 3-day Kp bars (SWPC) | B | **Live** |
| Band conditions: HF day/night outlook from SFI/Kp/A (local heuristic, not VOACAP) | B | **Live** |
| Solar imagery: NASA SDO AIA 193/304 and HMI, tap to cycle (F09) | B | **Live** |
| Space alerts: NOAA SWPC alerts, watches and warnings currently in force (scale, headline, issued/until; not on the default dashboard) | B | **Live** |
| DX cluster: telnet client, spot parser, spot list card | C | Partial (band/mode filters and watch-list highlight with ★ count on the card; no map overlay yet; no DXCC-entity watching yet; shows Not configured until a cluster host is set) |
| Callsign lookup (QRZ XML / HamQTH): typed call -> name, QTH, grid, country; lazy login, session re-login, in-memory cache | E | Partial (needs a Settings account; not yet wired to DX cluster spots; fixtures follow the documented XML, not live captures) |
| Sun & Moon: sun rise/set, day length (DE, DX), moon phase, rise/set, az/el at DE (offline) | D | Working (EME not started) |
| DE/DX current weather (Open-Meteo) | E | Working (conditions, wind, temperature, humidity, pressure, cloud cover, 3 h pressure trend; no forecast or popup yet) |
| Earthquakes M2.5+ past day (USGS): strongest event, count, recent list (pane + map overlay: dots sized and coloured by magnitude, labelled "M6.8", removed when data is not live) | H | **Live** |
| Tropical storms (NHC active storms: name, class, category, wind, pressure, movement, advisory time; pane only, track overlay waits for area G's API; JTWC not yet) | H | **Live** |
| NWS fire-weather alerts (api.weather.gov: Red Flag Warning, Fire Weather Watch, Fire Warning, Extreme Fire Danger; worst alert, area, issued/until, start time of a future watch, expired alerts dropped; nationwide, no region filter or map overlay yet; no Red Flag Warning was active when the fixture was captured, so it holds a Fire Weather Watch) | H | **Live** |
| NWS marine warnings and advisories (api.weather.gov: worst alert, area, issued/until times, counts by type, expired alerts dropped; nationwide, no region filter or map overlay yet) | H | **Live** |
| Satellites: next passes at DE for popular amateur birds (ISS, SO-50, AO-7, FO-29, AO-73, RS-44, AO-91, AO-85) from CelesTrak TLEs (daily, cached) via vendored SGP4; peak elevation >= 10 deg; no map track, sky plot, alerts or TLE-age limit yet | D | Partial (not on the default dashboard; fixture is a real capture) |
| NCDXF/IARU beacons: which beacon is on each of the five bands now (fixed schedule, offline; the five on-air beacons are marked on the map, positions from approximate locators) | D | Working |
| On the air: POTA + SOTA activator spots merged newest first (call, reference, frequency, mode, age; QRT dropped; POTA spots drawn on the map via the overlay API (SOTA has no position in its feed), only while live; no filters yet) | C | Partial (not shown on the default dashboard yet; fixtures are real captures) |
| Host sensors: CPU temperature (sysfs thermal zones), Wi-Fi signal dBm (/proc/net/wireless) and root-disk use; each metric hides when its source is absent, card shows unconfigured when none exist; no BME280 or history yet; not on the default dashboard | E | Partial |
| Everything else in the backlog | A–E | Not started |
