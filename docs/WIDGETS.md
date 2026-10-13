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
- **States:** call `setState()` / `trackSource()` so the card shows `live`, `stale` (with age), `loading`, `offline` or `unconfigured`. Never show an invented or stale-as-live value; missing values render as "—" (`Metric { available: false }`).
- **Settings and accounts:** read from `context.settings` (`AppSettings`). New settings go in `AppSettings` and `SettingsPage.qml` (UI area owns those files; other areas request additions in their PR description or add a small, separate field-only change). Passwords are C++-only, never QML properties.
- **UI:** build from `Card`, `Metric`, `StatusBadge`, `AppButton`, `FormField` and `Theme` tokens. Touch targets ≥ `Theme.touch`; everything reachable by keyboard (TV remote); text scales with `Theme.scale`.
- **Screenshots:** `hamclock-pro --config-dir DIR --fixtures src/widgets/<id>/tests/fixtures --theme dark --size 1024x600 --screenshot out.png` renders deterministically without the network. Check 800×480, 1024×600, 1280×800, 1920×1080, light and dark.
- **Sources:** only those in the roadmap's Data source policy or listed below; record terms and polling in the PR before first use.

## Areas and backlog

Each area: one owner agent, branches `agent/<area>-<slug>`, files listed are exclusive to that area.

### A. UI shell and design (owns `src/ui/`, `cmake/HamClockApp.cmake`, `docs/WIDGETS.md` contract section)
1. Match `design/dashboard.html` closely: header, sidebar, card spacing, typography, map toolbar; smooth touch scrolling; pressed/focus states.
2. TV mode: overscan inset setting, remote/keyboard focus order across cards, focus ring, large type.
3. Settings: locator entry (Maidenhead) as alternative to lat/lon, time-zone picker list, map click to set DX.
4. Map: full-card map, pan-free zoom presets, grayline band styling, DE/DX labels; then port the map drawing from QWidget to a pure QPainter renderer.
5. Port Big Clock, Stopwatch, Countdown, DE time settings dialogs into QML pages; then retire the Qt Widgets programs (`hamclock-pro-dashboard`, `-editor`, `-map-preview`).
6. Pane rotation (F26): user chooses which widgets show in which slot; layout saved.
7. Kiosk: systemd unit + `--fullscreen`, eglfs instructions; on-screen keyboard decision.

### B. Space weather and solar (owns `src/widgets/spaceweather/`, `solar*`, `bands*`)
1. ✅ Kp, solar flux, X-ray class (NOAA SWPC) — the reference widget.
2. Solar wind speed/density and Bz (SWPC `products/solar-wind/*`), with sparkline history.
3. Sunspot number and 30-day solar flux trend chart; Kp 3-day bar chart.
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
2. Moon phase, rise/set, elevation/azimuth at DE (F04) (offline).
3. Satellites (F05): CelesTrak TLEs (cached daily), SGP4 propagation (vendored library with licence recorded), next passes at DE, live ground track on map, up to two tracked satellites.
4. NCDXF/IARU beacons (F19): current transmitting beacon per band, beacon locations on map (offline schedule).
5. EME planning (F06): Moon position at DE and DX, mutual window.

### E. Weather, lookup, events, radio (owns `src/widgets/weather/`, `lookup/`, `events/`, `radio/`)
1. DE/DX weather (F10) via Open-Meteo (no key): temperature, wind, conditions, at both ends.
2. Callsign lookup (QRZ XML or HamQTH) using the account in Settings: name, grid, country for DX and spotted calls; session key handling, no password logging.
3. Contests (F17) and DXpeditions calendars; on-the-air events (F18) — check each feed's terms first.
4. Radio control (F20): read frequency/mode from rigctld/flrig on localhost or LAN (read-only first), show on header; rotator later. Safety review before any transmit-affecting command.
5. GPS (F21) via gpsd for DE location; CPU temperature/sensors (F22).

## Status

| Widget | Area | State |
|---|---|---|
| Station + clocks + DE local time | A | Working (QML) |
| World map, day/night, path | A | Working (QML, rendered via legacy widget) |
| Space weather: Kp, SFI, X-ray | B | **Live** |
| DX cluster: telnet client, spot parser, spot list card | C | Partial (no map overlay or filters yet; shows Not configured until a cluster host is set) |
| Sun rise/set, day length at DE and DX (offline) | D | Working (moon, satellites, beacons, EME not started) |
| Everything else in the backlog | A–E | Not started |
