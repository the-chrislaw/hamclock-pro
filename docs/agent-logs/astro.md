# Agent log: astro

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (UTC)
- Added the NCDXF/IARU beacons pane (backlog D4): `src/domain/astro_beacons.*` implements the published 3-minute / 10-second-slot schedule offline; `src/widgets/beacons/` shows the current beacon on 14.100/18.110/21.150/24.930/28.200 MHz (compact two-column layout). Tests check the schedule rule (4U1UN first slot on 14.100, one band up per slot, wrap, negative time). Beacon locators in the table are approximate and not yet used.
- Not merged by me: #73 (conflicts), #60 (cancelled macOS checks), #84 (CI pending) - other areas.
- Next: beacon map markers once area G's overlay API exists; then satellites (D3: vendor SGP4 + licence note in docs/).

## 2026-10-13 (UTC), run 2
- Satellites slice 1 (backlog D3): vendored Vallado SGP4 (via the MIT-licensed PyPI `sgp4` sdist, since GitHub downloads are blocked in the sandbox) in `src/domain/astro_sgp4/`, licence and one local patch (self-overlapping `strcpy`, found by ASan) recorded in `docs/third-party-sgp4.md`. `src/domain/astro_satellite.*` adds TLE parsing, TEME to sub-satellite point, az/el/range and a pass finder. Test checks the Spacetrack #3 object 88888 vectors at t=0 and t=360 min plus AO-7 structural checks from the CelesTrak fixture.
- No widget yet: next slice is the TLE fetch/cache (CelesTrak, daily) and a compact satellites pane with next passes; then map tracks via area G's overlay API.
- Others' PRs: #93 CI pending, #73 conflicting (not mine, left alone).

## 2026-10-14 (UTC)
- Satellites slice 2 (backlog D3): `src/widgets/satellites/` pane. CelesTrak amateur TLEs fetched daily through PolledSource (cached, offline/stale states), eight popular birds matched by name, next pass per bird at DE (>= 10 deg, 24 h horizon), five soonest shown as name / "in N min" or HH:MMZ / peak elevation (compact two-column layout). No rows are invented without a station or data. Test uses a six-set trim of the real CelesTrak capture. Rendered at 800x480, 1024x600, 1920x1080 light and dark offscreen (not on-device). Source terms recorded in the roadmap Data source policy.
- Next: map ground track via area G's overlay API once it exists, user-chosen satellites (max two tracked), TLE max age and pass alerts (D7); then EME (D5).
- Others' PRs: #103 CI pending, #73 not mergeable-checked (other areas, left alone).

## 2026-10-14 (UTC), run 2
- Beacons map markers (backlog D4): the five on-air NCDXF beacons are published as overlay layer "beacons" (diamond + callsign) via area G's overlay API; `ncdxf_beacon_location()` gives the locator centre. Test covers locator centres and the layer. Screenshot at 1024x600 dark checked offscreen (not on-device).
- Not mine: #73 conflicting, #111 CI pending; left alone.
- Next: satellite ground track on the map, user-chosen satellites, TLE age (D3/D7), EME (D5).

## 2026-10-14 (UTC), run 3
- Fixed QA issue #118 (Sun & Moon pane clipped Moonset): Moonrise/Moonset now share one "Moon rise/set" row, and Phase, rise/set and Now are dropped (last first) when the body cannot fit them instead of being sliced. Checked offscreen at 800x480, 1024x600, 1920x1080 (not on-device).
- Not mine: #127, #73 left alone.
- Next: satellite ground track on the map, user-chosen satellites, TLE age (D3/D7), EME (D5).

## 2026-10-14 (UTC), run 4
- Satellite ground track on the map (backlog D3): overlay layer "satellites" with each tracked bird's labelled sub-point and a dotted track (-10 to +50 min, 3 min steps) via area G's marker API (no polyline primitive exists, so dots). Test checks layer, labelled points and coordinate ranges; ASan/UBSan run on satellite and beacon tests.
- Not mine: #135, #73 left alone.
- Next: user-chosen satellites (max two), TLE age (D7), EME (D5).

## 2026-10-14 (UTC), run 5
- EME planning slice 1 (backlog D5): `src/domain/astro_eme.*` (mutual Moon-above-both-horizons window bisected to ~1 s, path loss vs perigee = 40 log10(d/356400 km)) and `src/widgets/eme/` compact pane (Moon az/el at DE and DX, mutual window, distance, path loss, declination; lower rows dropped, not clipped, when short). Test checks window edges against the ephemeris, antipodes (no window), in-progress windows and the 2.3 dB apogee figure. Rendered offscreen at 800x480, 1024x600, 1920x1080 light/dark (not on-device); ASan/UBSan run of the EME test clean.
- Merged #149 (green, other area). #154-#156 CI pending, left alone.
- Next: user-chosen satellites (max two), TLE age (D7), moon image in the Moon pane linking EME (D12), hams.at pane (D8, needs key).

## 2026-10-14 (UTC), run 6
- Next solar eclipse pane (backlog D9, slice 1): `src/domain/astro_eclipse.*` implements Meeus ch. 54 (greatest-eclipse time, gamma, type, partial magnitude) offline; `src/widgets/eclipse/` compact pane (next eclipse, greatest time, countdown, central/gamma or magnitude; lower rows dropped, not clipped). Test checks eight eclipses 2023-2027 against NASA catalogue times (within 5 min) and gamma (within 0.01) plus type; ASan/UBSan clean. NASA's eclipse site is not reachable from the sandbox, so reference values are from memory of the published catalogue and were not re-fetched this run. Rendered offscreen at 800x480, 1024x600, 1920x1080 (not on-device). Shows worldwide only, never claims visibility at DE.
- Not mine: #161, #164, #73 left alone (CI pending / other areas).
- Next: eclipse visibility at DE, user-chosen satellites (max two), TLE age (D7), moon image (D12), hams.at pane (D8, needs key).

## 2026-10-14 (UTC), run 7
- Maximum TLE age (backlog D7, slice 1): element sets with an epoch more than 30 days old are dropped from passes and the map layer; if all are stale the pane says "Element sets over 30 days old" instead of showing drifted orbits as live. Test advances the clock 60 days and back. Pane text only (no layout change). ASan/UBSan left to CI.
- Not mine: #173, #174 (CI pending), #73 left alone.
- Next: user-chosen satellites (max two), pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8, needs key).

## 2026-10-14 (UTC), run 8
- User-chosen satellites (backlog D3): the map layer now tracks at most two satellites (was all eight): `[satellites] tracked=AO-7,ISS` in options.ini (read-only, no AppSettings change), else the two with the soonest passes (first two without a station). Test covers default, choice, unknown names, cap. No settings control yet (shared settings UI, left for a later slice).
- Open PRs: #181, #182 (not mine, CI still running / failing tvOS check), #73 left alone. No `[D]` QA issues.
- Next: settings control for the choice, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8, needs key).

## 2026-10-14 (UTC), run 9
- Planets (backlog D10): `src/domain/astro_planets.*` (JPL approximate Keplerian elements, precessed to date) and `src/widgets/planets/` (compact pane of the planets above the horizon at DE, highest first; map layer "planets" with the five sub-planet points). Test checks Mars 2025 and 2027, Jupiter 2026 and Saturn 2025 oppositions and Venus inferior conjunction (2025-03-23) to within 2 days, Venus elongations (47.2 deg 2025-01-10, 45.9 deg 2025-06-01) and Mars distance 0.642 AU; ephemeris dates are from memory of almanac listings (my first recollections of two dates were wrong and the test dates were corrected to the published ones, the code was not adjusted to fit). Rendered offscreen at 800x480, 1024x600, 1920x1080 light/dark (not on-device).
- Not mine: #73 conflicting; #186, #189 have a failing macOS universal build, #190 pending; left alone.
- Next: settings control for tracked satellites, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8, needs key), yearly grayline chart (D11).
