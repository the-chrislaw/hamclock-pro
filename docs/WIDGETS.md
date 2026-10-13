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
- **States:** call `setState()` / `trackSource()` so the card shows `live`, `stale` (with age), `loading`, `offline` or `unconfigured`. Never show an invented or stale-as-live value; missing values render as "—" (`Metric { available: false }`).
- **Settings and accounts:** read from `context.settings` (`AppSettings`). New settings go in `AppSettings` and `SettingsPage.qml` (UI area owns those files; other areas request additions in their PR description or add a small, separate field-only change). Passwords are C++-only, never QML properties.
- **UI:** build from `Card`, `Metric`, `StatusBadge`, `AppButton`, `FormField` and `Theme` tokens. Touch targets ≥ `Theme.touch`; everything reachable by keyboard (TV remote); text scales with `Theme.scale`. Cards sit in clipped dashboard slots: read `Card.compact` (true in a small slot) and show key numbers only when it is set. The shell sets `Card.chooser` so the title opens the pane chooser; any widget id can be placed in any slot, whatever its registered `area`.
- **Screenshots:** `hamclock-pro --config-dir DIR --fixtures src/widgets/<id>/tests/fixtures --theme dark --size 1024x600 --screenshot out.png` renders deterministically without the network. Check 800×480, 1024×600, 1280×800, 1920×1080, light and dark.
- **Sources:** only those in the roadmap's Data source policy or listed below; record terms and polling in the PR before first use.

## Areas and backlog

Each area (A–F): one owner agent, branches `agent/<area>-<slug>`, files listed are exclusive to that area.

### A. UI shell and design (owns `src/ui/`, `cmake/HamClockApp.cmake`, `docs/WIDGETS.md` contract section)
0. 🟡 **Top priority: one-page slot layout** (done: no scrolling, three pane slots, chooser, choices saved; pending: per-slot rotation, side panes, `compact` layouts in each widget card). Remove page scrolling. Build the fixed slot grid (station column + pane row + map, scaling from 800×480 to 4K), a pane chooser (tap pane title → list of widgets; keyboard/remote accessible), per-slot choice saved in settings, optional rotation interval per slot. Default arrangement at 1024×600: station column left, three panes on top (Space weather, Solar wind, DX cluster), map filling the rest. Verify by screenshots at 800×480, 1024×600, 1280×800, 1920×1080 that nothing is clipped and nothing scrolls. Real-screen issues to fix: docs/hardware-validation.md.
0a. **Top priority now: dense small-screen layout + tap-to-change (owner, Oct 13, after seeing #70 on the 7-inch Pi).** The owner's screen is 1024×600 (also verify 800×480); every pixel matters, like the original's dense 800×480 layout.
   - **No header bar.** Remove the top "HamClock Pro / Hide path / Settings" bar on small screens (≤1280×800). Settings opens from a small gear icon in the station card's corner (and a long-press anywhere); Hide/Show path moves to a small toggle on the map card or the map tap menu.
   - **Tight spacing at compact scale:** outer margin and gaps ~4 px, card padding ~6–8 px, radius ~8 px, compact title row (smaller title, status shown as a coloured dot with an accessible label instead of the "Live" pill). Keep 44 px touch targets for buttons, not for decorative padding.
   - **Station column narrower** (~190 px at 1024 wide) with denser type; clock still the largest element.
   - **Map uses its whole card:** no side letterbox. Fill the card width; if the 2:1 map is too tall, crop polar latitudes (keep at least ±70°) rather than shrinking. Map title row removed or overlaid in a corner.
   - **Panes get the rest:** pane row height sized so each widget's compact layout fits without clipping (fix Space weather X-ray clipping and Solar wind Bz cut-off from docs/hardware-validation.md together with area B by defining a compact mode the widgets follow).
   - **Tap anywhere on a pane to change it** (not only the title), like the original: tap opens the pane chooser for that slot; swipe left/right on a pane steps to the previous/next widget; long-press opens that widget's own options when it has any. Keyboard/remote: Enter on a focused pane opens the chooser.
   - **Acceptance:** screenshots at 800×480 and 1024×600 (light/dark) show no header bar, no clipped text, the map filling its card, and content covering ≥95 % of the screen; then verified on the owner's Pi via the auto-updater.
0b. **Settings: location lookup.** "Find" button: search an address or place via OpenStreetMap Nominatim (honest User-Agent, max 1 request/second, only on button press; sample in tests/fixtures/sources), fills latitude, longitude, grid square and suggests the time zone; Maidenhead grid entry as an alternative; on macOS/iPad also "Use my location" via the OS location service (with area F).
1. Match `design/dashboard.html` closely: header, sidebar, card spacing, typography, map toolbar; smooth touch scrolling; pressed/focus states.
2. TV mode: overscan inset setting, remote/keyboard focus order across cards, focus ring, large type.
3. Settings: locator entry (Maidenhead) as alternative to lat/lon, time-zone picker list, map click to set DX.
4. Map: full-card map, pan-free zoom presets, grayline band styling, DE/DX labels; then port the map drawing from QWidget to a pure QPainter renderer.
5. Port Big Clock, Stopwatch, Countdown, DE time settings dialogs into QML pages; then retire the Qt Widgets programs (`hamclock-pro-dashboard`, `-editor`, `-map-preview`).
6. Pane rotation (F26): user chooses which widgets show in which slot; layout saved.
7. Kiosk: systemd unit + `--fullscreen`, eglfs instructions; on-screen keyboard decision.

### B. Space weather and solar (owns `src/widgets/spaceweather/`, `solar*`, `bands*`)
1. ✅ Kp, solar flux, X-ray class (NOAA SWPC) — the reference widget.
2. ✅ Solar wind speed/density and Bz (SWPC `json/rtsw/*_1m.json`; the old `products/solar-wind/*` URLs are gone), with sparkline history.
3. ✅ Sunspot number and 30-day solar flux trend chart; Kp 3-day bar chart (`solartrend`: SWPC `f107_cm_flux.json`, `observed-solar-cycle-indices.json`, `noaa-planetary-k-index-forecast.json`; polled every 6 h / 12 h / 30 min).
4. Band conditions (F07): day/night HF band outlook computed from SFI/Kp/A (document the model), optionally KC2G MUF map if terms allow.
5. Solar imagery (F09): NASA SDO latest images (AIA 193/304, HMI), cached, tap to cycle.
6. NOAA alerts/watches feed for storms and radio blackouts.

### C. Spots (owns `src/widgets/dxcluster/`, `pskreporter/`, `ota/`, `src/net/telnet*`)
1. DX cluster (F13): telnet client (DXSpider/AR-Cluster/CC-Cluster), callsign login, reconnect/backoff, spot parser, spot list card, spots on the map, band/mode filters.
2. PSKReporter (F14): reception reports for your callsign (respect its query-rate limits), map overlay.
3. POTA and SOTA activator spots (public APIs).
4. Watch lists (F16): highlight callsigns/prefixes/DXCC entities; alert badge.
5. Callsign prefix → DXCC entity/country lookup (F12) from a bundled, licensed prefix table.

### D. Astronomy, satellites, beacons (owns `src/widgets/moon/`, `satellites/`, `beacons/`, `src/domain/astro*`)
1. ✅ Sun rise/set at DE and DX, day length, grayline times (offline). Polar day/night handled.
2. ✅ Moon phase, rise/set, elevation/azimuth at DE (F04) (offline).
3. Satellites (F05): CelesTrak TLEs (cached daily), SGP4 propagation (vendored library with licence recorded), next passes at DE, live ground track on map, up to two tracked satellites.
4. NCDXF/IARU beacons (F19): current transmitting beacon per band, beacon locations on map (offline schedule).
5. EME planning (F06): Moon position at DE and DX, mutual window.

### E. Weather, lookup, events, radio (owns `src/widgets/weather/`, `lookup/`, `events/`, `radio/`)
1. ✅ DE/DX weather (F10) via Open-Meteo (no key): temperature, wind, conditions, at both ends.
2. 🟡 Callsign lookup (QRZ XML or HamQTH; card + session handling done, spotted-call wiring pending) using the account in Settings: name, grid, country for DX and spotted calls; session key handling, no password logging.
3. Contests (F17) and DXpeditions calendars; on-the-air events (F18) — check each feed's terms first.
4. Radio control (F20): read frequency/mode from rigctld/flrig on localhost or LAN (read-only first), show on header; rotator later. Safety review before any transmit-affecting command.
5. GPS (F21) via gpsd for DE location; CPU temperature/sensors (F22).

### F. Apple platforms: macOS and iPad (owns `src/platform/apple*`, `src/platform/credential_store*`, `cmake/Apple*.cmake`, `.github/workflows/apple.yml`, `packaging/apple/`)
Owner decision (Oct 13): ship a signed macOS app and an iPad app after the Pi 1.0 work is under way. The agent sandbox cannot run Xcode, so every change is verified on GitHub's macOS runners via `apple.yml`.
1. `apple.yml` CI: build and ctest on macOS (Homebrew Qt), and build the app for the iOS Simulator (Qt for iOS via `aqtinstall`, pinned version); upload the macOS app and a Simulator screenshot as artifacts.
2. Credential store: `src/platform/credential_store` with macOS/iOS Keychain and a 0600-file fallback elsewhere; AppSettings uses it (coordinate with area A in the PR).
3. macOS app polish: app icon, Info.plist (name, version, category), universal (arm64 + x86_64) build, `.dmg`; signing and notarization steps wired but skipped until the owner provides an Apple Developer ID (open "Owner input needed: Apple Developer account").
4. iPad: iOS CMake target, safe-area and orientation handling, touch-first layout check at iPad sizes (1180×820, 1366×1024), app lifecycle (pause polling and the DX cluster connection in background, resume cleanly), no QtWidgets dependency (requires area A's QPainter map renderer, backlog A4).
5. App Store readiness: privacy manifest, no private APIs, attribution screen for data sources (NOAA, NASA, Natural Earth, Open-Meteo…), TestFlight upload once the owner's account exists.

## Settings and accounts parity (checked against the original's setup.cpp, Oct 13)

Source: OpenHamClock `ESPHamClock/setup.cpp` (the fork current users run; some items postdate 4.22). Every setting below needs a place in Settings (area A owns the page; each area adds its own fields with minimal, separate changes) and works offline-safe when empty.

| Original setting | Purpose | Owner | Status |
|---|---|---|---|
| Callsign; DE lat/long (decimal or DD:MM:SS, N/S/E/W); grid square (auto from lat/long) | Station | A | Partial: decimal only; add DMS and grid entry (see A0b) |
| DX cluster host, port, **login with SSID (1–99)**, watch list, native commands sent after login, UDP/multicast option (e.g. WSJT-X) | Spots | C | Partial: host/port only |
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
| World map, day/night, path | A | Working (QML, rendered via legacy widget) |
| Space weather: Kp, SFI, X-ray | B | **Live** |
| Solar wind speed/density, IMF Bz/Bt with 1 h sparklines (SWPC RTSW) | B | **Live** |
| Solar trend: sunspot number, 30-day flux trend, 3-day Kp bars (SWPC) | B | **Live** |
| DX cluster: telnet client, spot parser, spot list card | C | Partial (band/mode filters on the card; no map overlay yet; shows Not configured until a cluster host is set) |
| Callsign lookup (QRZ XML / HamQTH): typed call -> name, QTH, grid, country; lazy login, session re-login, in-memory cache | E | Partial (needs a Settings account; not yet wired to DX cluster spots; fixtures follow the documented XML, not live captures) |
| Sun & Moon: sun rise/set, day length (DE, DX), moon phase, rise/set, az/el at DE (offline) | D | Working (satellites, beacons, EME not started) |
| DE/DX current weather (Open-Meteo) | E | Working (conditions, wind, temperature; no forecast yet) |
| Everything else in the backlog | A–E | Not started |
