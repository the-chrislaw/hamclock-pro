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

## 2026-10-14 (UTC), run 11
- Yearly sunrise/sunset series (backlog D11, domain slice): `sun_year_series(year, lat, lon)` in `astro_sun.*` returns per-day rise/set minutes (UTC) and polar state; test checks London June/December solstice rows against almanac times and Tromso midnight sun/polar night. No UI yet (chart pane is the next slice). Full ctest green locally; ASan/UBSan left to CI.
- Not mine: #73 conflicting, left alone. No `[D]` QA issues.
- Next: grayline chart pane, SatNOGS transmitters, settings control for tracked satellites, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8, needs key).
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

## 2026-10-14 (UTC), run 10
- Satellite detail box groundwork (owner request, D3): `LookAngle` gains `range_rate_km_s` (central difference over ±0.5 s; backward compatible) and `doppler_downlink_hz` / `doppler_uplink_hz` in `astro_satellite.*`. Test checks the rate against a 10 s finite difference and the formulas (7 km/s at 145.8 MHz ≈ 3.4 kHz, opposite senses for up/down). No UI change; transmitter data (SatNOGS) and the box itself are next slices.
- Not mine: #196/#197 have a failing iPad Simulator check, #73 conflicting; left alone. No `[D]` QA issues.
- Next: SatNOGS transmitter parse + NORAD join, settings control for tracked satellites, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8, needs key).

## 2026-10-14 (UTC), run 12
- SatNOGS transmitters (backlog D3 detail box, domain slice): `src/domain/astro_transmitters.*` parses the SatNOGS DB JSON (frequencies, mode, inverting flag, status, CTCSS tone from the description), joins to TLEs by NORAD id (`tle_norad_id`, `group_by_norad`) and maps an uplink frequency through a linear transponder (inverting reverses the offset). Test uses a trimmed real response (AO-7, SO-50, AO-91, ISS, FO-29) with the CelesTrak fixture; db.satnogs.org is reachable from the sandbox. No fetching or UI yet; data-source policy recorded in the roadmap.
- Not mine: #73 conflicting, left alone. No `[D]` QA issues.
- Next: fetch/cache transmitters, the detail box itself, settings control for tracked satellites, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8, needs key).

## 2026-10-14 (UTC), run 13
- Pass sky-plot data (backlog D6, domain slice): `SatellitePropagator::pass_track()` returns evenly spaced az/el samples from AOS to LOS; test checks sample count, endpoints on the horizon, azimuth range and peak vs the pass maximum (AO-7 fixture). No UI yet (sky plot in the DX panel is next).
- Not mine: #220 had no CI results yet, #73 conflicting; left alone. No `[D]` QA issues.
- Next: sky plot pane, fetch/cache transmitters, detail box, settings control for tracked satellites, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8).

## 2026-10-14 (UTC), run 14
- Pass sky plot (backlog D6, UI slice): `SatellitesModel::skyTrack` (24 az/el samples of the first listed pass) and a polar plot in the Satellites card (north up, zenith centre, rings at 30/60 deg) shown only when the slot has room (width > 240 and height > 150 theme units); compact slots keep the text list. The DX panel is area A/G's, so the plot lives in the pane for now. Test checks sample range, endpoints at the 10 deg pass threshold and peak vs the pass maximum. Rendered offscreen at 800x480, 1024x600, 1920x1080 light/dark (not on-device); ASan/UBSan clean for the model test.
- Not mine: #73 conflicting, left alone. No `[D]` QA issues.
- Next: fetch/cache transmitters, detail box, settings control for tracked satellites, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8), yearly grayline chart (D11).

## 2026-10-14 (UTC), run 15
- Yearly sunrise/sunset chart (backlog D11, UI slice): new `src/widgets/daylight/` (hamclock_add_widget takes one widget per folder, so the pane could not live in `sunmoon/`; same area D) draws weekly sunrise (amber) and sunset (blue) curves from `sun_year_series` for DE, minutes after 00:00 UTC, with today marked; gaps at polar day/night. Test checks London solstice rows against almanac times (within 4 min) and Tromso gaps. Rendered offscreen at 800x480, 1024x600, 1920x1080 (not on-device); no axis labels or local-time scale yet. ASan/UBSan left to CI.
- Not mine: #73 conflicting; #238/#239 CI still running; left alone. No `[D]` QA issues.
- Next: settings control for tracked satellites, fetch/cache transmitters + detail box, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8).

## 2026-10-14 (UTC), run 16
- Satellite detail box (owner request #246, D3): the Satellites model fetches the SatNOGS transmitters daily (cached, `satnogs-transmitters` source) and joins them by NORAD id; tapping a pass row replaces the list with a box: az/el with compass, range, next pass AOS/TCA/LOS (UTC) with AOS/LOS compass points and countdown, up to three active transmitters with Doppler-corrected downlink/uplink (updated each second), CTCSS and linear/inverting, SatNOGS CC BY-SA attribution when not compact. Tap again closes. Test uses the SatNOGS fixture via a URL-aware fake transport. Not done: sunlit/eclipsed, "more passes", map-marker tap and the G0 mechanism, ground-track/Set-as-DX actions. Screenshots and ASan/UBSan left to CI.
- Not mine: #245/#248 CI still running, #242 no checks, #73 conflicting; left alone.
- Next: remaining detail-box items, settings control for tracked satellites, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8).

## 2026-10-14 (UTC), run 16
- Found the SatNOGS fetch and detail box already on main (my parallel PR #251 conflicted and was closed as redundant). Kept only the useful difference: the transmitters URL is filtered to `service=Amateur&alive=true` (about 330 kB instead of the 4 MB full database); roadmap note updated. No logic change.
- Not mine: #73 conflicting; left alone.
- Next: remaining detail-box items (sunlit/eclipsed, Show track, Set as DX, map tap), settings control for tracked satellites, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8).

## 2026-10-14 (UTC), run 17
- Sunlit/eclipsed (owner request #246, D3 detail box): `sun_teme_km` (Astronomical Almanac low-precision Sun vector), `in_earth_shadow` (cylindrical shadow) and `SatellitePropagator::sunlit`; the detail box line shows "sunlit" / "eclipsed". Test checks the Sun direction at the 2000 equinox and declination at the 2024 June solstice, shadow geometry cases and a plausible LEO eclipse fraction over a day (88888). Penumbra ignored. Screenshots left to CI.
- Not mine: #256 CI still running, #73 conflicting; left alone.
- Next: Show track / Set as DX / more passes in the detail box, settings control for tracked satellites, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8).

## 2026-10-14 (UTC), run 18
- "More passes" (owner request #246, D3 detail box): the detail box lists the next four passes of the tapped satellite (day, HH:mmZ, peak elevation) on a wrapped "Then ..." line. The model keeps up to five passes per satellite from the existing search; the list rows are unchanged. Test checks at most four later passes with a time and a peak at or above the 10 degree threshold. Full ctest passes (138/138); screenshots and ASan/UBSan left to CI.
- Not mine: #73 conflicting; left alone. No other `[D]` QA issues.
- Next: Show track / Set as DX (needs G's tap mechanism), settings control for tracked satellites, pass alerts, eclipse visibility at DE, moon image (D12), hams.at pane (D8).

## 2026-10-14 (UTC), run 19
- Eclipse visibility at DE (backlog D9, domain + pane): `eclipse_visibility()` scans +-4 h around the global greatest eclipse (2 min steps), keeps samples with the Sun up and takes the minimum Sun-Moon topocentric separation; under 0.40 deg is Visible, 0.40-0.64 Borderline (Moon ephemeris is ~0.1 deg), else None. The Eclipse card shows an "At DE" row when a station is set. Test (2024-04-08): Dallas and Denver visible, London, Sydney (night), Moscow, Cairo and Tokyo not; invalid latitude throws. No local magnitude or contact times (needs Besselian elements). Screenshots/ASan left to CI.
- Not mine: #73 conflicting; #273/#274 CI still running. No new `[D]` QA issues.
- Next: pass alerts, settings control for tracked satellites, moon image (D12), hams.at pane (D8).

## 2026-10-14 (UTC), run 20
- Moon phase disc (backlog D12, partial): the Sun & Moon card draws the Moon as a Canvas disc from the elongation (crescent/gibbous terminator ellipse, lit side right when waxing, mirrored for a southern DE; no parallactic tilt, no texture). Shown only when the pane has room for the phase row; compact slots keep the text. Test checks elongation near 180 at the 2025-03-14 full moon and the southern flag. Rendered offscreen at 800x480, 1024x600, 1920x1080 (not on-device; no clipping, phase text fits). Not done: link to the EME tool.
- Not mine: #73 conflicting, #279/#282 CI still running at start; left alone. No new `[D]` QA issues beyond #246 (detail box items remaining need G's tap mechanism).
- Next: pass alerts, settings control for tracked satellites, hams.at pane (D8).
