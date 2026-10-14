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
0. 🟡 **Top priority: one-page slot layout** (done: no scrolling, three pane slots, chooser, choices saved; map now fills its item without the widget's inner margin; done: dense small-screen shell (no header, tight spacing, settings button in the station card) and tap-anywhere/swipe pane switching; done: long-press pane options (change, step, empty, reset); done: per-slot rotation (off/5/10/30/60 s, set from the long-press menu, saved) with custom rotation sets (long-press: add/remove the shown widget; empty set = all widgets); done: category picker (all registered widgets categorised, tested); pending: side panes, `compact` layouts in each widget card). Remove page scrolling. Build the fixed slot grid (station column + pane row + map, scaling from 800×480 to 4K), a pane chooser (tap pane title → list of widgets; keyboard/remote accessible), per-slot choice saved in settings, optional rotation interval per slot. Default arrangement at 1024×600: station column left, three panes on top (Space weather, Solar wind, DX cluster), map filling the rest. Verify by screenshots at 800×480, 1024×600, 1280×800, 1920×1080 that nothing is clipped and nothing scrolls. Real-screen issues to fix: docs/hardware-validation.md.
0c. 🟡 (done: tap the clock to swap UTC and DE local; DE and DX next sunrise/sunset countdown lines; tap the path line to toggle short/long path; Metric/Imperial distance units in Settings, saved; auxiliary time line: tap cycles day of year / JD / MJD / LST / UNIX; date order (D M Y / M D Y / ISO) and 12/24 h clock in Settings, saved; DX local time with an automatic zone from the DX coordinates (offline nearest-city hint), manual DX zone override pending) **Owner priority (Oct 13): station column parity with the original's DE/DX panels.** Do this right after A0, before 0b. Owner found the left column missing much of the original. Each item compact enough for the 7-inch 1024×600 column; verify by screenshot:
   - Callsign banner: call in large type with fg/bg colours (rainbow option), tap to cycle call / custom title / ON AIR (A13).
   - DE box: local time and date in selectable formats (digital 12/24 h, analog, calendar, annotated analog), DE sunrise and sunset shown as "in 3h 12m" / "ago" or "at" times (data from area D's astro code).
   - DX box: DX local time with automatic time zone from DX coordinates (manual override), DX sunrise/sunset, prefix → country name, short-path and long-path distance/bearing with a tap toggle, true or magnetic bearing (WMM), units Imperial/Metric.
   - Tap targets as in the original: tap DE or DX to set a new location (grid, lat/lng or search, reusing 0b), tap the clock for Big Clock / stopwatch / countdown, tap UTC to toggle UTC/local.
   - Auxiliary time line (date, day of year, JD/MJD, sidereal, UNIX) selectable (A14 subset).
0b. 🟡 (done: DMS and decimal coordinate entry, Maidenhead grid entry that fills DE, offline Nominatim reply parser `LocationTools`, "Find" button (one Nominatim request per press, max 1/s, fills lat/lon/grid); pending: zone suggestion, "Use my location") **Settings: location lookup.** "Find" button: search an address or place via OpenStreetMap Nominatim (honest User-Agent, max 1 request/second, only on button press; sample in tests/fixtures/sources), fills latitude, longitude, grid square and suggests the time zone; Maidenhead grid entry as an alternative; on macOS/iPad also "Use my location" via the OS location service (with area F).
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
13. Callsign banner (done: tap cycles call / ON AIR / 73 DE call): call / rotating titles / ON AIR text, fg/bg colours, rainbow option.
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
7. ✅ DRAP pane and DRAP map style done (`solardrap`: SWPC `text/drap_global_frequencies.txt`, highest affected frequency, publishes the `drap` map style while the table is under 2 h old, polled every 10 min; not on the default dashboard).
8. ✅ Aurora pane and Aurora map style done (`solaraurora`: SWPC OVATION `json/ovation_aurora_latest.json`, peak probability per hemisphere, publishes the `aurora` map style while the grid is under 3 h old, polled every 10 min; not on the default dashboard). The automatic switch to Aurora/DRAP is wired: `spaceweather` feeds fresh Kp and X-ray flux to `set_auto_space_weather` (stale values are passed as unknown); the settings toggle to choose it arrives with the map styles UI.
9. ✅ DST (Disturbance) pane (`solardst`: SWPC `products/kyoto-dst.json`, latest hourly Dst with Quiet/Weak/Moderate/Intense/Super word, 24 h low and 48 h plot; polled every 30 min; hidden when the latest record is over 6 h old; not on the default dashboard).
10. ✅ NOAA R/S/G scales pane (`solarscales`: SWPC `products/noaa-scales.json`, current R/S/G, 24 h max and tomorrow's outlook; polled every 10 min; not on the default dashboard).
11. ✅ X-ray 24 h history plot (`spaceweather`: SWPC `xrays-1-day.json`, 10-minute peaks on a log A1–X10 axis with class lines; polled every 15 min, hidden when out of date; compact card shows a thin version).
12. ✅ Space-weather stats ranked by impact (`solarimpact`: the SWPC R/S/G levels from `noaa-scales.json`, worst first with a one-line HF/aurora/GPS effect from the NOAA scale definitions; polled every 10 min; not on the default dashboard). The Kyoto Dst index (`kyoto-dst.json`, polled every 30 min) joins the ranking as a fourth row "D" while its latest hourly value is under 6 h old (Quiet 0, Weak 1, Moderate 2, Intense 3, Super 5). Kp and X-ray are not added: the G and R scales are already derived from them.
13. (Third slice: `[bandsmatrix] mode = ssb|cw|digital` and `power_w` (1..1500) in the options file set the pane's mode and power; no in-app UI yet. Second slice: the `bandsmatrix` pane shows the DE to DX matrix, band by UTC hour for 24 h, from SWPC SFI/Kp, SSB 100 W fixed; settings for power/mode/antenna, long path and the MUF/REL map are next. First slice: `bands/path_model` is the offline in-house path model, owner chose this over VOACAP/hosted services in #281: hops, foF2/MUF from SFI and the sun at control points, D-layer absorption, SNR by power/gain/mode, Kp penalty; approximate, tested; the matrix card, settings for power/mode/antenna and the MUF/REL map are next.) VOACAP-style DE→DX reliability matrix (power, mode, takeoff angle, SP/LP, UTC/DE timeline, antenna models) plus per-band TOA/REL and MUF maps; needs a source agreement or an offline open-source model.
14. ✅ HamQSL (N0NBH) HF and VHF conditions pane (`solarhamqsl`: `https://www.hamqsl.com/solarxml.php`, calculated day/night HF band words and VHF openings, credit shown; polled hourly; hidden when the report is over 6 h old; not on the default dashboard).
15. Tropo map style (check source terms first).

### C. Spots (owns `src/widgets/dxcluster/`, `pskreporter/`, `ota/`, `src/net/telnet*`)
1. DX cluster (F13): telnet client (DXSpider/AR-Cluster/CC-Cluster), callsign login, reconnect/backoff, spot parser, spot list card, spots on the map, band/mode filters.
2. PSKReporter (F14): reception reports for your callsign (respect its query-rate limits), map overlay.
3. POTA and SOTA activator spots (public APIs).
4. Watch lists (F16): highlight callsigns/prefixes/DXCC entities; alert badge.
5. ✅ (table + lookup + country role on DX cluster rows; not shown in the card yet) Callsign prefix → DXCC entity/country lookup (F12) from a bundled, licensed prefix table (`dxcluster/dxcc_table`, AD1C cty.dat, MIT).

**Audit additions (C):**
6. WSPR and RBN in Live Spots: of DE / by DE, by call or grid, max distance or count, band statistics, map paths.
7. More On-The-Air programmes (WWFF, IOTA, GMA, LLOTA, WWTOTA, TOWERS, WWBOTA) with age/sort/mode/band filters.
8. HamAlert pane. 9. Nearby APRS pane (APRS-IS, radius filter, receive-only) — ✅ first slice (card, fixed 100 km; settings and map overlay later). 10. Meshtastic pane and map links. 11. Band-activity heatmap (band × continent) — ✅ first slice (counts over the DX cluster's recent spots, shown as a "Busiest band" line in the DX cluster card; standalone heatmap pane, PSK/WSPR/RBN sources later).
12. UDP spot input (WSJT-X, N1MM/DXLog, Log4OM), multicast, "UDP sets DX", "by me"/"all".
13. Cluster login with SSID and native commands after login.
14. Spot labels None/Prefix/Call/Dot, minimum label distance, per-source map on/off; map overlays for ONTA, ADIF and HamAlert.
15. 🟡 (ADIF map overlay of logged stations at their grid centre done; NADXCC/NABAND watch keywords done, NAPref/NAGrid not) ADIF log pane with file watching and not-in-log watch keywords (NADXCC/NAPref/NABand/NAGrid).
16. 🟡 (xOTA reference parsed from comments and shown as a badge on DX cluster rows; IOTA reference names not done) IOTA reference names and xOTA markers parsed from comments.

### D. Astronomy, satellites, beacons (owns `src/widgets/moon/`, `satellites/`, `beacons/`, `src/domain/astro*`)
1. ✅ Sun rise/set at DE and DX, day length, grayline times (offline). Polar day/night handled.
2. ✅ Moon phase, rise/set, elevation/azimuth at DE (F04) (offline).
3. 🟡 (SGP4, look angles with range rate and Doppler formulas, pass search, detail box with later passes, TLE download, next-passes pane and map sub-points with ground track done; the map tracks at most two satellites, chosen by `[satellites] tracked=` in options.ini or else the two soonest passes; a settings control for the choice next) Satellites (F05): CelesTrak TLEs (cached daily), SGP4 propagation (vendored library with licence recorded), next passes at DE, live ground track on map, up to two tracked satellites.
4. ✅ NCDXF/IARU beacons (F19): current transmitting beacon per band, the five on-air beacons marked on the map (offline schedule).
5. 🟡 (offline pane done: Moon az/el at DE and DX, next mutual window, distance and path loss vs perigee; moon image/tool link and per-band sky temperature not done) EME planning (F06): Moon position at DE and DX, mutual window.

**Audit additions (D):**
6. 🟡 (sky plot of the next pass in the Satellites pane done; DX panel placement not) Satellite pass sky plot in the DX panel, co-visibility planner, group schedule, frequency table and status links.
7. 🟡 (maximum TLE age of 30 days and an on-card rise alert 5 min before AOS done) Satellite rise/set alert; user-supplied TLE.
8. hams.at upcoming activations pane. 9. 🟡 (worldwide next eclipse pane done, offline; Visible/Borderline/Not visible at DE done, approximate; local magnitude and contact times not done) Next solar eclipse pane. 10. ✅ Planets on the map (offline): Mercury to Saturn sub-planet points as overlay layer "planets", plus a pane of the planets above the horizon at DE; Uranus and Neptune not included. 11. 🟡 (offline chart pane `daylight` done: weekly sunrise/sunset curves in UTC with today marked; DE local time and axis labels not done) Yearly sunrise/sunset ("grayline") chart.
12. 🟡 (phase disc in the Sun & Moon pane done: drawn from the elongation, lit side mirrored for southern DE, no parallactic-angle tilt or surface texture; link to the EME tool not done) Moon pane image with phase, linked to the EME tool. 13. Launches pane. 14. Balloons pane.

**Owner request (Oct 14): satellite detail box.** Tapping a satellite (map marker or Satellites pane row) opens a small, non-modal box using area G's tap-for-details mechanism (G0); closes on tap outside or Back. Contents, compact at 1024×600:
- Now: azimuth/elevation, range, above/below horizon, sunlit/eclipsed, range rate.
- Next pass: AOS / TCA / LOS times (local and UTC), max elevation, AOS and LOS azimuths with compass letters, countdown; "more passes" lists the next 5.
- Listen/Transmit: each transmitter's downlink and uplink frequency and mode (FM, linear/inverting, CW beacon, packet/APRS), CTCSS tone where the database has it, and live Doppler-corrected frequencies (from SGP4 range rate, updated each second; uplink and downlink corrected in opposite senses, inverting transponders handled).
- Status: active/inactive from the database; show "frequency data from SatNOGS DB (CC BY-SA 4.0)" attribution.
- Actions: show ground track + footprint on the map; Set as DX (sub-satellite point).
Data: transmitters from SatNOGS DB API (`https://db.satnogs.org/api/transmitters/?format=json`, open data, CC BY-SA 4.0; poll at most daily, cache, honest User-Agent), joined to TLEs by NORAD id. Tests: Doppler maths against a hand-computed case, NORAD join, inverting-transponder sign, fixture-based parse. If `db.satnogs.org` is not reachable, open "Owner input needed: allowlist domains".

### E. Weather, lookup, events, radio (owns `src/widgets/weather/`, `lookup/`, `events/`, `radio/`)
1. ✅ DE/DX weather (F10) via Open-Meteo (no key): temperature, wind, conditions, at both ends.
2. 🟡 Callsign lookup (QRZ XML or HamQTH; card + session handling done, spotted-call wiring pending) using the account in Settings: name, grid, country for DX and spotted calls; session key handling, no password logging.
3. Contests (F17) and DXpeditions calendars; on-the-air events (F18) — check each feed's terms first.
4. 🟡 (read-only rigctld frequency/mode card done, with a read-only flrig XML-RPC fallback on localhost:12345; header display pending) Radio control (F20): read frequency/mode from rigctld/flrig on localhost or LAN (read-only first), show on header; rotator later. Safety review before any transmit-affecting command.
5. GPS (F21) via gpsd for DE location; CPU temperature/sensors (F22).

**Audit additions (E):**
6. Weather parity: humidity, pressure and trend, clouds, city, DX time zone; brief weather popup on new DE/DX.
7. Weather and clouds map styles; Windy and WEFAX links.
8. Hazard panes and map overlays: quakes (USGS), tropical storms (NHC/JTWC), NWS marine and fire-weather warnings, lightning with range rings, active fires (NASA FIRMS); check each feed's terms first.
9. Radio: PTT-driven ON AIR, then (after a safety review) tune to a tapped spot; flrig and rigctld.
10. 🟡 (read-only rotctld position card done; tracking the selected satellite or DX pending the radio/rotator safety review) Rotator pane (rotctld) that can track the selected satellite or DX.
11. BME280 environment panes (temperature, pressure, humidity, dew point; corrections); CPU temperature, Wi-Fi RSSI and disk-use history.
12. NTP choice; gpsd/NMEA for time as well as location; IP geolocation.
13. NetLogger active nets pane.
14. 🟡 (output interface with null and fake backends done; no hardware backend yet) GPIO outputs/inputs (alarm, countdown lights, ON AIR, satellite alarm) behind a `src/platform` interface, Linux/Pi only.
15. 🟡 (read-only aircraft count card from the local PiAware aircraft.json done; map overlay pending) PiAware ADS-B link.

### F. Apple platforms: macOS and iPad (owns `src/platform/apple*`, `src/platform/credential_store*`, `cmake/Apple*.cmake`, `.github/workflows/apple.yml`, `packaging/apple/`)
Owner decision (Oct 13): ship a signed macOS app and an iPad app after the Pi 1.0 work is under way. The agent sandbox cannot run Xcode, so every change is verified on GitHub's macOS runners via `apple.yml`.
1. ✅ `apple.yml` CI: build and ctest on macOS (Homebrew Qt), and build the app for the iOS Simulator (Qt for iOS via `aqtinstall`, pinned version); upload the macOS app and a Simulator screenshot as artifacts.
2. ✅ (store + tests; AppSettings wiring pending area A) Credential store: `src/platform/credential_store` with macOS/iOS Keychain and a 0600-file fallback elsewhere; AppSettings uses it (coordinate with area A in the PR).
3. 🟡 (Info.plist, category, version, app icon, unsigned dmg done; signing/notarization steps wired in apple.yml and skipped until secrets exist; universal build job green; the signed/notarized dmg is now built from it) macOS app polish: app icon, Info.plist (name, version, category), universal (arm64 + x86_64) build, `.dmg`; signing and notarization steps wired but skipped until the owner provides an Apple Developer ID (open "Owner input needed: Apple Developer account").
4. 🟡 (app lifecycle: `AppLifecycle` + PolledSource pause/resume done; DX cluster/rigctld/gpsd connections and safe-area insets done; iPad layout check pending) iPad: iOS CMake target, safe-area and orientation handling, touch-first layout check at iPad sizes (1180×820, 1366×1024), app lifecycle (pause polling and the DX cluster connection in background, resume cleanly), no QtWidgets dependency (requires area A's QPainter map renderer, backlog A4).
5. App Store readiness: privacy manifest, no private APIs, attribution screen for data sources (NOAA, NASA, Natural Earth, Open-Meteo…), TestFlight upload once the owner's account exists.

**Audit additions (F):**
6. Mark Pi-only hardware items (GPIO, I2C, light sensor, kiosk power menu) as unavailable on macOS/iPad; check any remote-API listener against App Store rules.

**iPhone support (owner request, Oct 14; after the first iPad TestFlight build):**
F10. Make the iOS target universal (iPhone + iPad, same bundle ID com.chrislaw.hamclockpro), portrait and landscape, safe areas, Dynamic Island/notch insets; TestFlight builds install on iPhone. Area A provides the phone layout (A0d); area F wires the target, orientation, App Store metadata and a phone Simulator screenshot when the Simulator flow works.
A0d (area A). Phone layout for widths under ~600 pt: station strip on top (callsign, UTC/local, DE/DX grids), one pane at a time with swipe/page dots plus the pane chooser, map below (landscape: map left, one pane right), same Map button/menu; no text truncation (smoke guard at 390×844 and 844×390).

**More Apple surfaces (owner request, Oct 14; start only after the signed macOS app and the iPad TestFlight build ship):**
7. Widgets (WidgetKit extension in the iPad/iPhone app): small/medium widgets for band conditions, Kp/SFI/X-ray and the next satellite pass. The same widgets appear in StandBy and on the CarPlay dashboard (CarPlay supports iPhone widgets), which is the App Store-acceptable way to put HamClock in the car. A full CarPlay app is not planned: CarPlay apps are limited to Apple's approved categories (audio, navigation, communication, EV charging, parking, fuelling, quick food ordering, driving task), so a dashboard would be rejected.
8. Apple TV app: Qt has no tvOS support, so a native SwiftUI front end over the shared C++ core (parsers, astro/SGP4, models) built as a static library for tvOS. Remote-friendly focus navigation, 1080p/4K layouts, the same one-page dashboard. Design doc first (owner review), then a prototype.
9. HomeView (Apple's wall/tabletop home hub announced Oct 13, 2026): a wall-mounted touchscreen dashboard is HamClock's natural home. Track whether Apple opens it to third-party apps or widgets; if only widgets, item 7 covers it. Open "Owner input needed" when the SDK story is clear.

### G. Map and overlays (owns `src/map/`, `src/ui/map*`, `src/ui/qml/Map*`, `src/qt/world_map_widget*`, `src/widgets/mapoverlay*`) — split from A on Oct 13
Takes over map work from area A (A4, A9, A10, and the map part of A0a) so layout and map proceed in parallel.
**Owner priority (Oct 13): the map is the biggest gap against the original. Spots on the map, projections + tap, grids/zones and propagation styles are all wanted, in parallel.** Order below is chosen so other areas can start drawing on the map as early as possible.
0. 🔴 (slices 1-3 done: Map button, layer/night/path/label toggles, saved; tap empty map -> Set DE/Set DX popup; base style choice + rotation in the menu, saved; grids, projection, marker detail popup remain) **Owner priority (Oct 14): map controls on the map, as in the original.** The original's core interaction is the map's own buttons/menu, not separate panes. Today the overlays exist (named layers with a `visible` flag) but there is no way to toggle them, so do this before the remaining G items:
   - A small "Map" button in a corner of the map card (large enough to tap on the 7-inch screen, keyboard/remote accessible) opening a Map View menu.
   - Layers, each on/off independently of which panes are shown: DX cluster spots, PSKReporter/RBN/WSPR, POTA/SOTA, satellites, NCDXF beacons, quakes, storms, other hazards, DE/DX path, night shading, labels (None/Prefix/Call/Dot).
   - Base style (Countries/Terrain and the styles registered through G3), with style rotation on/off.
   - Grid: None, Tropics, Lat/Lng, Maidenhead, Azimuth rings, CQ zones, ITU zones (entries appear as G5 implements them).
   - Projection (Mercator / Azimuthal / Azim One / Robinson as G4 implements them) and centre/zoom/reset.
   - Tapping a marker shows its details (call, frequency, age, distance/bearing) with Set DX; tapping empty map offers Set DE / Set DX.
   - All choices saved in options.ini and restored at start. Screenshot the open menu at 1024×600 and 800×480.
   - Gestures on every platform (owner request, Oct 14), zooming around the gesture point and clamped to 1–4×: pinch with two fingers on touchscreens (Pi touch displays, iPad) via PinchHandler; trackpad pinch on macOS (native zoom gesture, handled by the same PinchHandler); mouse wheel / two-finger scroll zoom on desktop; double-tap to zoom in, two-finger tap to zoom out; drag to pan. Keep the +/−/reset buttons for single-touch screens, TV remotes and keyboard. Test the zoom-about-point maths; note that many cheap HDMI touch panels are single-touch, so buttons stay the fallback.
   Widgets still own their data; a layer's pane is optional (e.g. quakes can be on the map with no Earthquakes pane).
1. Port the map from the QWidget renderer to a pure QPainter/Qt Quick renderer (unblocks the iPad build); map fills its card (crop polar latitudes rather than letterbox). Keep the PR small; land it before anything else.
2. ✅ (markers, labels, min distance, DE path; tap/legend later) Overlay API (moved up): a small interface other areas use to draw their objects (spots, satellites, APRS, hazards, beacons) with labels None/Prefix/Call/Dot and a minimum label distance, plus great-circle path drawing from DE. Document it in `docs/map-overlays.md` with one worked example. Areas B, C, D, E and H start their map items as soon as this merges.
3. ✅ (registry, rotation, auto space-weather pick, legend; settings UI comes with the first image style) Map styles framework (moved up): selectable base styles with rotation 20/60/90/120 s and automatic space-weather style, and a colour-scale legend. Individual image styles (DRAP, MUF, aurora, clouds…) come from the owning areas via this framework.
4. ✅ (zoom 1–4×, drag pan, reset, tap → `tapped(lat, lon)`, Set DE/DX popup, centre longitude; pure azimuthal-equidistant and Robinson maths with tests are in `src/map/projection.*`, selectable in the Map menu "Projection" row: Flat / Azimuthal (DE-centred) / Robinson / Azim one (near hemisphere only, rim 90° from DE), drawn whole through one `MapFrame` transform shared by base map, overlays and taps; no zoom/pan there) Projections: Mercator with centre longitude, Azimuthal (DE-centred), Azimuthal-one, Robinson; zoom 1–4×, recenter/reset; tap popup Set DE / Set DX.
5. Grids and zones: Tropics, lat/lng grid, Maidenhead fields/squares, azimuth rings, CQ and ITU zones, borders and US states, cities (licensed data), DE antipode and sublunar point (done: Map menu "Moon / antipode"), long-path drawing (done: Map menu "Long path"), gray mode (done: Map menu "Gray map"), country borders (done: Map menu "Country borders", Natural Earth 110m; US states and cities remain).

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

### I. Apple TV (owns `apple/tvos/`, `cmake/TvOS*.cmake`, `docs/apple-tv*.md`, `.github/workflows/tvos.yml`, `docs/agent-logs/tvos.md`)
Native SwiftUI tvOS app over the shared C++ core (Qt has no tvOS support); design in [docs/apple-tv.md](apple-tv.md). Unsigned Simulator builds only until the owner approves signing (#58).
1. 🟡 Design doc for owner review (written; awaiting owner comments).
2. ✅ `tvos.yml`: build the C++ core for the tvOS Simulator and run its unit tests on macOS.
3. 🟡 Minimal SwiftUI app: clock + live NOAA SWPC space weather (C ABI `hcpcore.h`, XcodeGen project), Simulator screenshot artifact (first CI run pending).
4. Remaining dashboard panes, map with shared overlay data, settings (after the design review).

### QA. Quality reviewer (no feature ownership; writes only `docs/agent-logs/qa.md`, `docs/hardware-validation.md` notes and GitHub issues)
Runs every 4 hours: reviews PRs merged since its last run, builds main, renders screenshots at 800×480 and 1024×600 (light/dark) with all widget fixtures, compares behaviour against the original (docs/FEATURE-AUDIT.md) and the contract (one page, no clipping, compact panes, honest states, portability), and opens a focused GitHub issue per problem labelled `qa` with the owning area in the title (e.g. "[B] Space weather X-ray clipped at 1024x600"). Agents treat open `qa` issues in their area as top of their backlog. Never edits feature code.

## Settings and accounts parity (checked against the original's setup.cpp, Oct 13)

Source: OpenHamClock `ESPHamClock/setup.cpp` (the fork current users run; some items postdate 4.22). Every setting below needs a place in Settings (area A owns the page; each area adds its own fields with minimal, separate changes) and works offline-safe when empty.

| Original setting | Purpose | Owner | Status |
|---|---|---|---|
| Callsign; DE lat/long (decimal or DD:MM:SS, N/S/E/W); grid square (auto from lat/long) | Station | A | Partial: decimal only; add DMS and grid entry (see A0b) |
| DX cluster host, port, **login with SSID (1–99)**, watch list, native commands sent after login, UDP/multicast option (e.g. WSJT-X) | Spots | C | Partial: host/port, login SSID (1–99) and receive-only post-login commands done; watch list and UDP spot input (N1MM/DXLog `<spot>` XML or plain "DX de" lines, off by default, localhost or LAN) done; WSJT-X binary protocol, multicast, "UDP sets DX" missing |
| **HamAlert.org** telnet login + password | Personal spot alerts | C | Partial: user/password saved in the owner-only credentials file (password C++ only), pane connects to hamalert.org:7300; map overlay done (entity centres, live only; Settings toggle not wired); trigger names and per-alert filters missing |
| **APRS-IS** server (default rotate.aprs2.net), port 14580, search radius; receive-only login with callsign | Nearby APRS stations on the map | C | Partial: card lists nearest stations (plain and compressed positions); server, port and radius (1–500 km) set in Settings (options.ini `[aprs]`, reconnects on change); map overlay done |
| **hams.at API key** | Personalised satellite pass info | D | Missing |
| gpsd host **or** serial NMEA device | DE location/time from GPS | E | Partial (gpsd display only; DE/time use and serial NMEA pending) |
| NTP server | Time source | E | Missing |
| rigctld, rotctld, flrig host/port | Radio and rotator | E | Partial (read-only radio; defaults only, no settings yet) |
| PiAware ADS-B receiver host | Aircraft on the map | E | Partial (count card from localhost:8080 only; no settings, no map overlay) |
| ADIF file path + watch list; On The Air watch list | Log and activity highlighting | C/E | Partial |
| Meshtastic node IDs (neighbour links) | Mesh map overlay | C | Missing |
| BME280 sensors (I2C device, temp/pressure corrections) | Local environment | E | Partial (kernel IIO driver; corrections via options.ini `[sensors] temp_offset_c`/`pressure_offset_hpa`, no Settings control yet) |
| Brightness dim/bright %, display schedule | Display | A | Planned (F27) |
| Mercator map centre longitude; colour choices | Map/appearance | A | Missing |
| Local passwords guarding restart/exit/unlock/new DE-DX/configurations | Kiosk protection | A | Missing |
| QRZ/HamQTH lookup | *Not in original*: our addition | E | Planned |

Credentials (HamAlert password, hams.at key, lookup password) are C++-only and stored by the credential store (Keychain on Apple; on Linux/Pi an owner-only 0600 JSON file in a 0700 directory, stored in clear text, so it is readable by any process of the same user and ends up in backups or SD-card images of the data directory; a libsecret/Secret Service store is a possible later addition), never shown back or logged.

## Status

| Widget | Area | State |
|---|---|---|
| Station + clocks + DE local time | A | Working (QML); dashboard page scrolls (touch flick) when widgets exceed the screen, map keeps a full 2:1 size |
| World map, day/night, path | G | Working (Map button + View menu: per-layer on/off, night shade, path, label mode, saved in options.ini [map]; base style + rotation entries (shown once styles are published); projection (flat/azimuthal/Robinson/Azim one) and grid entries; QML, pure QPainter renderer, no QtWidgets; styles registry with legend (no image styles published yet); fills its card, crops polar rows to at least ±70°; the legacy QWidget map remains only for the old dashboard) |
| Space weather: Kp, SFI, X-ray | B | **Live** |
| Solar wind speed/density, IMF Bz/Bt with 1 h sparklines (SWPC RTSW) | B | **Live** |
| Solar trend: sunspot number, 30-day flux trend, 3-day Kp bars (SWPC) | B | **Live** |
| Band conditions: HF day/night outlook from SFI/Kp/A (local heuristic, not VOACAP) | B | **Live** |
| HF path reliability: DE to DX band-by-hour matrix for the next 24 h from the in-house path model (approximate, not VOACAP; not on the default dashboard) | B | **Live** |
| Solar imagery: NASA SDO AIA 193/304 and HMI, tap to cycle (F09) | B | **Live** |
| NOAA scales: R/S/G radio blackout, radiation and geomagnetic storm levels with tomorrow's outlook (SWPC; not on the default dashboard) | B | **Live** |
| Space alerts: NOAA SWPC alerts, watches and warnings currently in force (scale, headline, issued/until; not on the default dashboard) | B | **Live** |
| HamQSL (N0NBH) calculated HF day/night band conditions and VHF openings (not on the default dashboard) | B | **Live** |
| Dst index: latest hourly Dst (Kyoto WDC via SWPC) with storm word, 24 h low and 48 h plot (not on the default dashboard) | B | **Live** |
| D-RAP: highest affected frequency and the D-RAP map style (SWPC; not on the default dashboard) | B | **Live** |
| Aurora: peak OVATION aurora probability per hemisphere and the Aurora map style (SWPC; not on the default dashboard) | B | **Live** |
| Space weather impact: R/S/G levels ranked worst first with HF/aurora/GPS effect (SWPC; not on the default dashboard) | B | **Live** |
| DX cluster: telnet client, spot parser, spot list card | C | Partial (bundled DXCC prefix table, `country` role on rows, shown before the comment in the card (country only when compact); band/mode filters and watch-list highlight with ★ count on the card; spots drawn on the map at the DXCC entity centre (one marker per entity, newest first, prefix labels, DE path, only while the cluster link is live; `DxClusterModel::setMapVisible` is the per-source hook, Settings toggle not wired); DXCC-entity watching via `@PREFIX` watch entries (e.g. `@VP8`, cty.dat primary prefix); shows Not configured until a cluster host is set) |
| Callsign lookup (QRZ XML / HamQTH): typed call -> name, QTH, grid, country; lazy login, session re-login, in-memory cache; shows distance and short-path bearing from DE to the grid centre | E | Partial (needs a Settings account; not yet wired to DX cluster spots; fixtures follow the documented XML, not live captures) |
| Sun & Moon: sun rise/set, day length (DE, DX), moon phase, rise/set, az/el at DE (offline) | D | Working |
| Next solar eclipse: date, type, greatest-eclipse time and countdown, worldwide (Meeus ch. 54, offline, checked against NASA catalogue values; yes/no visibility at DE from the Moon ephemeris, no local magnitude or contact times) | D | Partial (not on the default dashboard) |
| Planets: Mercury, Venus, Mars, Jupiter, Saturn above the horizon at DE (elevation, azimuth) and their sub-planet points on the map (JPL approximate elements, about 0.2°, offline; checked against almanac oppositions and elongations; not on the default dashboard) | D | Working |
| EME: Moon az/el at DE and DX, next mutual window (Moon above both horizons), distance and path loss vs perigee (offline) | D | Partial (not on the default dashboard; geometric visibility only) |
| DE/DX current weather (Open-Meteo) | E | Working (conditions, wind, temperature, humidity, pressure, cloud cover, 3 h pressure trend, today's high/low; no multi-day forecast or popup yet) |
| Earthquakes M2.5+ past day (USGS): strongest event, count, recent list (pane + map overlay: dots sized and coloured by magnitude, labelled "M6.8", removed when data is not live; nearest quake to DE) | H | **Live** |
| Tropical storms (NHC active storms: name, class, category, wind, pressure, movement, advisory time; pane + map overlay: diamonds sized and coloured by category, labelled with the storm name, removed when data is not live; official forecast positions (12 h-5 day, from the NHC forecast advisory text) as small dots coloured by forecast intensity, tap for time and wind; no cone or line (overlay API has none), JTWC not yet) | H | **Live** |
| NWS fire-weather alerts (api.weather.gov: Red Flag Warning, Fire Weather Watch, Fire Warning, Extreme Fire Danger; worst alert, area, issued/until, start time of a future watch, expired alerts dropped; nationwide plus an "At DE" row from the same API's `point=` query for the saved station, no map overlay yet; no Red Flag Warning was active when the fixture was captured, so it holds a Fire Weather Watch) | H | **Live** |
| NWS marine warnings and advisories (api.weather.gov: worst alert, area, issued/until times, counts by type, expired alerts dropped; nationwide, plus a line for the marine region nearest DE with its expiry; no map overlay yet) | H | **Live** |
| Active fires (NASA FIRMS VIIRS NOAA-20, contiguous US + Hawaii, last 24 h: detection count, high-confidence count, strongest FRP and position, newest overpass; pane + map overlay: triangles for the 60 strongest non-low-confidence detections, removed when data is not live; nearest non-low-confidence detection to the saved DE (distance, bearing, overpass time); no global/regional choice yet; keyless public CSV, so no FIRMS MAP_KEY is used) | H | **Live** (fixture is a real trimmed capture) |
| Satellites: next passes at DE for popular amateur birds (ISS, SO-50, AO-7, FO-29, AO-73, RS-44, AO-91, AO-85) from CelesTrak TLEs (daily, cached) via vendored SGP4; peak elevation >= 10 deg; map track for at most two birds (options.ini choice, no settings control yet), sky plot of the next pass when the slot has room, no alerts yet; elements over 30 days old are dropped | D | Partial (not on the default dashboard; fixture is a real capture); SatNOGS transmitters fetched daily and joined by NORAD id; tapping a pass row opens a detail box (look angle, pass, transmitters with Doppler); no sunlit/eclipsed, map-marker tap or actions yet |
| NCDXF/IARU beacons: which beacon is on each of the five bands now (fixed schedule, offline; the five on-air beacons are marked on the map, positions from approximate locators) | D | Working |
| On the air: POTA + SOTA activator spots merged newest first (call, reference, frequency, mode, age; QRT dropped; POTA spots drawn on the map via the overlay API (SOTA has no position in its feed), only while live; display filter by program/band/mode/max age, applied to list, count and map; Settings fields in the Spots card save it to options.ini `[ota]`) | C | Partial (not shown on the default dashboard yet; fixtures are real captures) |
| PSKReporter: who heard my callsign in the last 15 min (count, best SNR, newest receivers with grid, band, mode, SNR, age); queried every 5 min only when a callsign is set, shows Not configured otherwise; receivers drawn on the map (grid-square centre, path from DE, only while live; `PskModel::setMapVisible` is the per-source hook, Settings toggle not wired); not on the default dashboard; fixture is a real capture | C | Partial |
| ADIF log: QSO count and newest QSOs from a local .adi file (path in Settings, read-only, reloaded when the file changes; unconfigured when empty); `worked(call)` ignores /P and EA8/ parts; not on the default dashboard; `AdifModel::entityNeeded/bandSlotNeeded` answer NADXCC/NABand (DXCC entity and entity+band not yet in the log; false when the log is not loaded or the entity is unknown), shown as a NEW DXCC / NEW BAND badge on DX cluster rows (resolved at run time through the registry; `NADXCC`/`NABAND` watch-list keywords highlight such spots); no NAPref/NAGrid or map overlay yet; parser tested on synthetic ADIF (no real log available) | C | Partial |
| HamAlert: personal spot alerts over telnet (hamalert.org:7300, `DX de` lines); `TelnetClient` gained an optional password step (sent once after a `password:` prompt, never logged); Not configured until user and password are saved; compact card (count + newest, list when not compact); receive-only; not on the default dashboard; tested with a fake socket, not against the live service | C | Partial |
| Host sensors: clock sync status (chronyc/timedatectl, read-only, via src/platform/ntp_status), CPU temperature (sysfs thermal zones), Wi-Fi signal dBm (/proc/net/wireless) and root-disk use; each metric hides when its source is absent, card shows unconfigured when none exist; BME280/BMP280 via kernel IIO sysfs (air temperature, pressure, humidity, dew point; not tested on a Pi; pressure trend after 30 min of in-memory history; no corrections or persistent history yet); not on the default dashboard | E | Partial |
| GPS (gpsd JSON on localhost:2947, read-only): fix quality, position, Maidenhead grid, satellites in use; unconfigured when gpsd is absent; does not change DE yet; "Clock agrees with GPS (within 2 s)" / "Clock is N s ahead of or behind GPS" check from the TPV time (display only, does not set the clock); NMEA 0183 GGA/RMC sentence parser (checksummed, unit-tested) with a line splitter and a freshness-checked GGA/RMC tracker exists but no serial reader or time use yet; tested against a loopback fake gpsd, not a real receiver; not on the default dashboard | E | Partial |
| Radio (rigctld on localhost:4532, strictly read-only: only `f` and `m` are ever sent; falls back to flrig XML-RPC on localhost:12345 asking only `rig.get_vfo`/`rig.get_mode`): frequency, band, mode; unconfigured when rigctld is absent, error replies are never shown as live; tested against a loopback fake rigctld, not a real rig; no flrig, not on the header or default dashboard | E | Partial |
| Rotator (rotctld on localhost:4533, strictly read-only: only `p` is ever sent): azimuth and elevation; unconfigured when rotctld is absent, error replies and out-of-range values are never shown as live; tested against a loopback fake rotctld; not run with a real rotator; no tracking or move commands; not on the default dashboard | E | Partial |
| ADS-B (PiAware/dump1090 aircraft.json on localhost:8080, GET only): aircraft heard in the last 60 s and how many have a position; unconfigured without a receiver, a receiver clock more than 60 s off is stale, garbage is an error; loopback fake-server test, not run against a real receiver; not on the default dashboard | E | Partial |
| Everything else in the backlog | A–E | Not started |
