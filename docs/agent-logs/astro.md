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

## 2026-10-14 (UTC), run 21
- Pass alert (backlog D7, partial): `pass_alert_text` gives "NAME rises in N min" within 5 min of AOS and "NAME above horizon" during a pass; the Satellites model exposes it as `alert` and the card shows it as an accent line, with the matching row highlighted. No sound/notification and no settings for the lead time yet. Unit test covers the boundaries; satellites tests pass. No screenshot (alert only shows when a pass is imminent, which the fixtures do not produce); ASan/UBSan left to CI.
- Not mine: #288/#289 CI still running at start, #73 conflicting; left alone. No new `[D]` QA issues.
- Next: settings control for tracked satellites, user-supplied TLE, hams.at pane (D8).

## 2026-10-14 (UTC), run 22
- Show/Hide track (D3 detail box; stands in for the settings control): the detail box has a "Show track on map" / "Hide track on map" action that adds or removes the satellite in `[satellites] tracked=` (max two, oldest drops off) and republishes the map layer. Pure `toggle_tracked_names` plus a model test of the flip and the saved ini value. Full ctest passes (142/142); no screenshot (the detail box needs a tap); ASan/UBSan left to CI.
- Not mine: #73 conflicting; #294/#296/#297/#298 CI still running at start. No new `[D]` QA issues.
- Next: Set as DX (needs G's tap mechanism), user-supplied TLE, hams.at pane (D8).

## 2026-10-14 (UTC), run 23
- Launches pane (backlog D13, first slice): new `launches` widget parses Launch Library 2 upcoming launches (hourly, cached, Data source policy updated); card shows the next launch with a T- countdown (past launches dropped after a 2 h grace), provider, site, status and the next three in non-compact slots; approximate times marked. Shared changes: `launches` added to the Sky & Space category in PaneChooser.qml; the `--fixtures` transport now ignores a trailing slash in the URL path (backward compatible). Full ctest passes (144/144); rendered offscreen at 800x480 and 1024x600 and 1920x1080 (not on-device), no truncation. ASan/UBSan left to CI.
- Not mine: #306 CI running, #73 conflicting; left alone. No new `[D]` QA issues.
- Next: balloons pane (D14), user-supplied TLE, hams.at pane (D8, needs API docs), launch pad map markers.

## 2026-10-15 (UTC), run 24
- Merged #319 (spots/space PR, green). User-supplied TLE (backlog D7, partial): `user-tles.txt` beside options.ini (3-line or 2-line sets) is merged over the CelesTrak download by NORAD id via `merge_user_tles`; user sets are not limited to the popular-bird list and still obey the 30-day age cut-off. Unit test covers replace and append. Full ctest passes (145/145); no UI change. ASan/UBSan left to CI.
- Not mine: #73 conflicting; #320/#321 CI still running at start. Balloons (D14) blocked on #314.
- Next: settings editor for tracked/user TLE, hams.at pane (D8, needs API docs/key), Set as DX.

## 2026-10-15 (UTC), run 25
- QA #325 (daylight year chart): the sunrise/sunset paths now break where the time wraps past 00:00 UTC (jump > 720 min between adjacent days) instead of drawing a vertical stroke. QML-only; rendered offscreen at 1024x600 for DE 40.7 N 74.0 W (not on-device): the blue series splits cleanly, no vertical strokes. No unit test (canvas paint logic).
- Not mine: #328 CI still running at start, #73 conflicting; left alone.
- Next: settings editor for tracked/user TLE, hams.at pane (D8, needs API docs/key), Set as DX.

## 2026-10-15 (UTC), run 26
- Daylight year chart axes (backlog D11, partial): hour guides at 6Z/12Z/18Z with labels and month initials along the bottom, only when the pane has room (compact slots stay bare). QML only. Rendered offscreen at 800x480, 1024x600, 1920x1080 dark and light with a New York DE (not on-device): labels legible, no clipping. Not done: DE local time axis.
- Not mine: #73 conflicting; left alone. No `[D]` QA issues besides #246 (needs G's tap mechanism); balloons still blocked on #314.
- Next: settings editor for tracked/user TLE, hams.at pane (D8, needs API docs/key), Set as DX.

## 2026-10-15 (UTC), run 27
- Merged #338 (space docs, green). Uranus and Neptune (backlog D10 completion): JPL approximate elements added to `astro_planets`; both appear in the planets pane (when above the horizon) and as map markers (layer now has seven). Tests: Uranus oppositions 2025-11-21 and 2026-11-25, Neptune 2025-09-23 (within 2 days), Neptune distance 28.9 AU, Uranus about 18.6 AU. Full ctest otherwise passes; no layout change (list rows). ASan/UBSan left to CI.
- Not mine: #73 conflicting; #339/#340 CI running at start. Balloons still blocked on #314.
- Next: settings editor for tracked/user TLE, hams.at pane (D8, needs API docs/key), Set as DX.

## 2026-10-15 (UTC), run 28
- Eclipse local circumstances (backlog D9, partial): `eclipse_visibility` now also returns the time of maximum at DE (minute resolution) and the magnitude there (fraction of the Sun's diameter, from the Sun-Moon separation and the Moon's distance); the card gains a "Local" row ("Max 18:39Z, 100% of Sun") only when visible/borderline at DE and the pane has room. Tests: 2024-04-08 Dallas (magnitude > 0.95, maximum within 5 min of 18:42Z), Denver partial magnitude 0.6-0.9, London zero. Maximum is a few minutes off NASA's value (ephemeris limit). Full ctest passes except nothing new failing; rendered 800x480, 1024x600, 1920x1080 only for the not-visible case (the next eclipse, 2027-02-06, is not visible from the test DE), so the new row itself was not screenshotted. ASan/UBSan left to CI. Not done: contact times.
- Not mine: #344 (spots) CI still running at start, #73 conflicting; left alone. Balloons still blocked on #314.
- Next: settings editor for tracked/user TLE, hams.at pane (D8, needs API docs/key), Set as DX.

## 2026-10-15 (UTC), run 29
- hams.at activations pane (backlog D8, first slice): new `hamsat` widget parses the public `hams.at/api/alerts/upcoming` endpoint (documented at hams.at/api/docs; reachable from the sandbox; no key needed to read; Data source policy updated). Card shows the next activation with a countdown, mode, frequency, grid and AOS-LOS, plus the next three in non-compact slots; finished passes dropped by the clock. Fixture is a trimmed real response. Shared change: `hamsat` added to the Sky & Space category in PaneChooser.qml. Rendered offscreen at 800x480, 1024x600 and 1920x1080 dark and light (not on-device); 1024x600 viewed, no clipping. hamsat tests pass; ASan/UBSan left to CI. Not done: API key / personalised passes (settings row), map markers.
- Not mine: #348/#349 CI still running at start, #73 conflicting; left alone. Balloons still blocked on #314.
- Next: settings editor for tracked/user TLE, hams.at key + personalised passes, Set as DX.

## 2026-10-15 (UTC), run 30
- Daylight year chart DE local time (backlog D11): `daylight_points` takes an optional IANA zone; minutes are shifted by the offset at 12:00 UTC of each day (QTimeZone, no hand-rolled DST) and wrapped, polar gaps kept; the model exposes `localTime` and the card tags guides 6L/12L/18L (Z when no zone). Tests: New York June (EDT) and December (EST) sunrise/sunset against almanac within 5 min, bad zone falls back to UTC, polar gap survives. Daylight tests pass; full ctest and ASan/UBSan left to CI. No screenshot of the pane itself (not on the default dashboard); the QML pane test passes.
- Not mine: #73 conflicting; left alone. No new `[D]` QA issues. Balloons still blocked on #314.
- Next: settings editor for tracked/user TLE, hams.at key + personalised passes, Set as DX.

## 2026-10-15 (UTC), run 31
- Eclipse contact times (backlog D9, partial): `eclipse_visibility` now walks outward from the local maximum in 20 s steps to give first and last partial contact at DE (marked when the horizon cuts the eclipse off at sunrise/sunset); the card gains a "Partial" row ("17:22 – 19:59Z") when the pane has room. Test: Dallas 2024-04-08 against NASA (17:23 and 20:02 UTC, within 8 min), Denver contacts bracket the maximum, London none. Full ctest passes (151/151). No screenshot of the new row (the next eclipse, 2027-02-06, is not visible from the test DE); QML mirrors the existing rows. ASan/UBSan left to CI. Not done: annular/total contact times (C2/C3).
- Not mine: #364 (spots) CI running at start, #73 left alone. Balloons still blocked on #314.
- Next: settings editor for tracked/user TLE, hams.at key + personalised passes, Set as DX.

## 2026-10-15 (UTC), run 32
- Docs only: the Status row for Satellites still said "no sunlit/eclipsed"; the detail box already shows it (`SatellitePropagator::sunlit`, covered in satellites_regression). Row corrected. No code change.
- Not mine: #371/#372 CI still running at start, #73 conflicting; left alone. Balloons blocked on #314. Remaining D items need shared surfaces (settings editor for TLE, hams.at key via credential store, Set as DX, map-marker tap from G) or hardware-grade ephemeris (annular/total contacts), so no larger slice was safe this run.
- Next: settings editor for tracked/user TLE, hams.at key + personalised passes, Set as DX once G's tap mechanism lands.

## 2026-10-15 (UTC), run 33
- Map tap details (backlog D, #246 first slice): G's `markerTapped` / tap menu now exists, so the satellite, planet and beacon overlay markers carry a `detail` line (satellite: name and altitude; beacon: frequency and location; planet: name). Tests extended in satellites_regression, beacons_regression, planets_regression. Not done: full satellite detail box on tap (SatNOGS transmitters, Doppler, passes). Not rendered on a screen; the popup is G's existing menu. ASan/UBSan left to CI.
- Not mine: #383 had no checks at start, #73 conflicting; left alone. Balloons blocked on #314.
- Next: satellite detail box from the tap menu, settings editor for TLE, hams.at key, annular/total contacts.

## 2026-10-15 (UTC), run 34
- Merged #389 (iPad QML plugins, green). Satellite detail box action (backlog D, #246): "Set as DX (sub-satellite point)" in the Satellites detail box, via `SatellitesModel::setAsDx()` (explicit tap only; DE, callsign and zone untouched; error text shown if no satellite is open). Test extended in satellites_model_regression. Full ctest 154/154 passes; not rendered on a screen; ASan/UBSan left to CI. Not done: footprint on the map.
- Not mine: #390 CI running at start, #73 conflicting; left alone. Balloons blocked on #314.
- Next: footprint, settings editor for TLE, hams.at key, annular/total contacts.

## 2026-10-15 (UTC), run 35
- Satellite footprint (backlog D, #246): `footprint_ring()` gives the horizon circle (angular radius acos(R/(R+h))) as 48 faint dots in the "satellites" map layer for each tracked bird (overlay API has markers only, so a dotted ring rather than a polygon). Tests: equator ring is a 20.3 deg circle, near-pole longitudes stay in range, bad input empty. Full ctest 154/154 passes; not rendered on a screen; ASan/UBSan left to CI.
- Not mine: #393 (tvos) CI running at start, #73 conflicting; left alone. Balloons blocked on #314.
- Next: settings editor for TLE, hams.at key, annular/total contacts.

## 2026-10-15 (UTC), run 36
- Marker tap opens the satellite detail box (backlog D, #246): `SatellitesModel::selectByName()` opens (never toggles) the detail for the listed satellite matching the tapped marker's label; the map tap menu (shared `MapTapMenu.qml`, small backward-compatible change: remembers the marker layer, adds a "Details" button only for layer "satellites") calls it through `widgetRegistry.model("satellites")`. Tests: unknown name opens nothing, case-insensitive match, re-select keeps open. Full ctest 154/154 passes; not rendered on a screen (the box shows in the Satellites pane, so the pane must be on screen); ASan/UBSan left to CI.
- Not mine: #403/#404 (other areas) and #73 conflicting left alone. Balloons blocked on #314; hams.at key blocked on #400.
- Next: settings editor for TLE, annular/total contacts.

## 2026-10-15 (UTC), run 37
- User TLE editor (backlog D7): `SatellitesModel::userTles()` / `saveUserTles(text)` validate pasted element sets (rejects text with no valid set, nothing written; empty text removes the file), save atomically to `user-tles.txt` beside options.ini and reload. The Satellites card gets a "My TLEs…" link (roomy slots only) opening a paste editor with Save / Clear / Cancel. Tests extended in satellites_model_regression (reject, save, clear). ctest satellites and ASan/UBSan satellites tests pass; not rendered with the editor open (the offscreen pane test has no data, so the link is hidden there); not on-device.
- Not mine: #73 conflicting, left alone. Balloons blocked on #314; hams.at key blocked on #400.
- Next: annular/total contact times (needs better ephemeris), launches ham filtering, EME sky temperature.

## 2026-10-15 (UTC), run 38
- EME Moon-Sun separation (backlog D5, partial): `moon_sun_separation_deg()` (geocentric, from the Moon's ecliptic latitude and elongation); the EME summary gains "N° from Moon" with "(sun noise)" under 10°, and a "Sun" row in the card when there is room. Tests: full Moon ~180°, 2025-03-29 new Moon eclipse day under 3°, first quarter ~90°. Per-band sky temperature still not done (needs a galactic background map I cannot validate offline). Not rendered on a screen; ASan/UBSan left to CI.
- Not mine: #417 has a failing macOS universal build, #73 conflicting; left alone. Balloons blocked on #314; hams.at key blocked on #400.
- Next: annular/total contacts, launches ham filtering.

## 2026-10-16 (UTC), run 39
- Launches ham hint (backlog D13, partial): `launches::ham_hint()` flags rideshare/cubesat/amateur and ISS-visit launches from name, mission and mission type only (a hint, not a payload claim); shown in the detail line and as a star on later rows. Tests: Bandwagon hinted, Starlink/MMX not, crew and amateur cases. Launches ctest passes, full ctest run; not rendered on a screen; ASan/UBSan left to CI.
- Not mine: #421 (tvos) CI still running at start, #73 conflicting; left alone. Balloons blocked on #314; hams.at key blocked on #400.
- Next: annular/total contacts, pad markers on the map.

## 2026-10-16 (UTC), run 40
- Launch pad markers (backlog D13): `launches::pad_position()` maps the location text to an approximate position for about 25 spaceports (unknown sites get no marker, nothing guessed); the model publishes a "launches" map layer, one marker per site labelled with the mission, tap detail "name · T-countdown", removed when the list is stale or empty. Tests: Vandenberg/Jiuquan positions, unknown site, layer present with one marker per site, layer removed on stale. Full ctest run; not rendered on a screen; ASan/UBSan left to CI.
- Not mine: #425 and #73 left alone. Balloons blocked on #314; hams.at key blocked on #400.
- Next: annular/total contacts, Moon pane EME link.

## 2026-10-16 (UTC), run 41
- Co-visibility planner, domain slice (backlog D6): `SatellitePropagator::mutual_windows()` finds windows when a satellite is above a minimum elevation at two stations at once (20 s steps, bisected edges). Tests: windows have both stations above the horizon, identical stations give exactly the pass list, zero windows requested. Satellites ctests pass; no UI yet (next: show the next DE+DX window in the detail box); not rendered; ASan/UBSan left to CI.
- Not mine: #73 conflicting, left alone. Balloons blocked on #314; hams.at key blocked on #400.
- Next: show the mutual window in the satellite detail box, annular/total contacts.

## 2026-10-16 (UTC), run 42
- Co-visibility in the satellite detail box (backlog D6): the detail map gains "mutual" (next DE+DX window within 48 h, "none in 48 h" otherwise), shown as a "DE+DX ..." line in roomy slots only; only when DX is set and differs from DE; computed once per 10-minute bucket, not per second. Test extended in satellites_model_regression. Satellites ctests pass; full ctest and ASan/UBSan left to CI; not rendered on a screen.
- Not mine: #432/#433 CI running at start, #73 conflicting; left alone. Balloons blocked on #314; hams.at key blocked on #400.
- Next: annular/total contacts, Moon pane EME link.

## 2026-10-16 (UTC), run 43
- EME next perigee/apogee (backlog D5, partial): `next_lunar_apsis()` finds the next distance extreme from the ephemeris (3 h scan, ternary refine); the EME card gains an "Apsis" row (nearer of next perigee/apogee, UTC date and distance) in roomy slots. Tests: 2025-11-05 supermoon perigee within 3 h of ~22:30 UTC and ~357,000 km, apogee mid-November, result strictly after the start. Time is only good to about an hour (flat curve). Not rendered on a screen; ASan/UBSan left to CI.
- Not mine: #73 conflicting, left alone. Balloons blocked on #314; hams.at key blocked on #400. Annular/total contacts skipped again: lunar ephemeris (~0.1 deg) is too coarse for second/third contact.
- Next: Moon pane EME link (needs an approved URL), per-band sky temperature.

## 2026-10-16 (UTC), run 44
- Next equinox/solstice (extra to backlog D1, no owner input needed): `season_event()` / `next_season_event()` in `astro_sun` (Meeus ch. 27 mean terms plus 24 periodic terms, TT minus 69 s); the Sun & Moon card gains a "Next season" row in roomy slots ("December solstice 21 Dec 20:50 UTC"). Tests: all four 2026 events within 3 min of almanac times, rollover into the next year, summary text. Full ctest 157/157 passes; ASan/UBSan left to CI; not rendered at the three sizes (only the existing offscreen pane test ran); not on-device.
- Not mine: #73 conflicting, left alone. Balloons blocked on #314; hams.at key blocked on #400; EME link/sky temperature on #440.
- Next: annular/total contacts remain blocked on ephemeris accuracy; little unblocked backlog left.

## 2026-10-16 (UTC), run 45
- Moon disc tilt (backlog D12, partial): `bright_limb_position_angle()` (Meeus ch. 48) and `moon_bright_limb_tilt_deg()` (adds the parallactic angle); the Sun & Moon phase disc is rotated to the bright-limb direction at DE, which replaces the old north/south mirror. Tests: Meeus example 48.a chi = 285.0 within 0.2 deg, waxing Moon lit on the right from London. Full ctest 156/156 passes; ASan/UBSan left to CI. The Sun & Moon card is not on the default dashboard, so the disc was not rendered at 800x480/1024x600/1920x1080 (screenshots only show the default layout); not on-device.
- Not mine: #73 conflicting, left alone. Balloons blocked on #314; hams.at key blocked on #400; EME link/sky temperature on #440.
- Next: annular/total contacts remain blocked on ephemeris accuracy; little unblocked backlog left.

## 2026-10-16 (UTC), run 46
- Satellite status link (backlog D6, partial): the detail box gains a "Satellite status (SatNOGS DB) ↗" link by NORAD id, opened in the system browser only (no new fetch), roomy slots only. Test: detail carries the link. Satellites ctests pass; full ctest and ASan/UBSan left to CI; not rendered on a screen.
- Not mine: #73 conflicting, left alone. Balloons blocked on #314; hams.at key blocked on #400; EME link/sky temperature on #440.
- Next: group schedule / frequency table; little unblocked backlog left.

## 2026-10-17 (UTC), run 47
- Satellite group schedule (backlog D6, partial): `pass_schedule_summary()` counts every pass in the next 24 h across the listed satellites and names the best one ("7 passes in 24 h, best AO-7 78° at 15:05Z"); shown beside "My TLEs…" in roomy slots only. Counts are a lower bound (at most 5 passes per satellite are searched). Tests: counting, best pick, expired and out-of-horizon passes excluded, singular, empty. Not rendered on a screen (card is not on the default dashboard); not on-device.
- Not mine: #73 left alone. Balloons blocked on #314; hams.at key blocked on #400; EME link/sky temperature on #440.
- Next: frequency table; little unblocked backlog left.

## 2026-10-17 (UTC), run 48
- Satellite frequency column (backlog D6, partial): `primary_downlink_text()` gives each listed row the first active SatNOGS downlink ("145.9200 MHz FM", or a "lo-hi MHz SSB" range for a transponder; empty when unknown); the roomy Satellites card shows it as a fourth column, compact slots unchanged. Tests: skips inactive, transponder range, none. Full ctest 156/156 passes; ASan/UBSan left to CI; not rendered on a screen (card is not on the default dashboard); not on-device.
- Not mine: #73 left alone. Balloons blocked on #314; hams.at key blocked on #400; EME link/sky temperature on #440.
- Next: DX-panel placement of the sky plot; little unblocked backlog left.

## 2026-10-17 (UTC), run 49
- Owner answered #440: EME tool link as a QR code on kiosk / link elsewhere (build), sky temperature won't-do, eclipse contact times via truncated ELP2000-82B (build). Recorded the sky-temperature won't-do in WIDGETS D5 and ROADMAP "On the radar" (docs only).
- Blocked this run: fetching the vendored QR encoder (Nayuki QR-Code-generator, MIT) from raw.githubusercontent.com was denied by the sandbox's permission check, so no encoder or ELP data was vendored. Needs the owner to allow the fetch or commit the files under `src/domain/astro_qr/` with their licence.
- Next: QR encoder + EME link; ELP2000-82B truncated theory and NASA-validated contacts.

## 2026-10-17 (UTC), run 50
- The raw.githubusercontent.com fetch worked this run, so the Nayuki QR encoder (MIT) is vendored under `src/domain/astro_qr/` (unmodified, own warning-free library) with a wrapper `astro_qr.{hpp,cpp}` (`qr_encode`) and docs/third-party-qr.md. Tests (`widget-eme-qr_regression`): version-1 size, three finder patterns, timing patterns, dark module, determinism, empty result when too long. No decoder was available, so scan-ability is not verified. Eme/QR ctests pass; full ctest and ASan/UBSan left to CI; no UI change yet.
- Not wired into the EME card: I cannot reach the original's EME link or verify a public planner URL from the sandbox, so I will not guess one. Asked the owner to pick the URL (comment on #440).
- Not mine: #73 conflicting, #471 CI running; left alone. ELP2000-82B tables not fetched yet. Balloons blocked on #314; hams.at key on #400.
- Next: EME card QR/link once the URL is chosen; ELP2000-82B truncated theory and NASA-validated contacts.

## 2026-10-18 (UTC), run 51
- Owner allowlisted cdsarc.cds.unistra.fr (#475). Vendored a truncated ELP2000-82B (D9 groundwork): `src/domain/astro_elp.{hpp,cpp}` and generated `astro_elp/elp82b_terms.inc` (1,252 main + 671 perturbation terms, 3e-8 rad cut, ~48 kB), docs/third-party-elp.md. Tests (`widget-eclipse-elp_regression`): five dates against the full theory (a direct port of elp82b.f run offline) within 0.2 arcsec / 0.2 km, Meeus 47.a within 0.005 deg, agreement with the coarse series. Evaluator is not wired into the Eclipse card yet.
- Not done: second/third contact solver and NASA 2024-04-08 / 2027-08-02 validation (next slice; needs nutation, deltaT and a Besselian-style shadow axis). EME QR link: I could not fetch the original's moonpane.cpp (the path I tried 404s); still needs the URL on #440.
- Next: contact solver on `elp_moon`; EME URL.

## 2026-10-19 (UTC), run 52
- Eclipse contact solver (backlog D9, partial): `eclipse_contacts()` in `astro_eclipse_contacts.cpp` finds C1-C4, maximum, magnitude at a station from `elp_moon` (precessed to the date, nutation applied), the Meeus ch. 25 Sun with aberration, WGS84 observer and a fixed 69 s delta-T; NASA's umbral limb radius is used for C2/C3. Tests (`widget-eclipse-eclipse_contacts_regression`): at NASA's greatest-eclipse points for 2024-04-08 and 2027-08-02 totality lasts 266 s (ref 268) and 383 s (ref 383), maximum within 60 s; 2017-08-21 is asserted only loosely (duration 66 s vs 160 s) because the low-accuracy Sun puts the path about 27 arcsec (55 km) from NASA's. Not "a few seconds": the Sun series is the limit, so the card is not wired to it yet. Full ctest and ASan/UBSan left to CI; no UI change; not on-device.
- Not mine: #73 left alone. Balloons blocked on #314; hams.at key on #400; EME QR link still waiting for the URL on #440.
- Next: a sharper Sun (VSOP87 truncation; may need a domain allowlist issue), then wire contacts into the Eclipse card.

## 2026-10-19 (UTC), run 53
- Sharper Sun (D9, partial): vendored a truncated VSOP87D Earth (156 terms, CDS VI/81, `astro_vsop`, generator in `src/domain/astro_vsop/`, docs/third-party-vsop.md); `eclipse_contacts()` now uses it (FK5 correction, nutation, aberration). Test `widget-eclipse-vsop_regression` vs Meeus 25.b (longitude 0.01 arcsec; radius within 1e-6 AU, Meeus's 8-digit value differs by 5.5e-7 from the truncated sum). Eclipse ctests pass; full ctest and ASan/UBSan left to CI.
- Honest result: the NASA comparison did not tighten. 2024 still 269 s vs 268 (max -2 s); 2027 max -59 s, 382 vs 383 s; 2017 duration 74 s vs 160 s. So the Sun was not the main limit; the remaining error is elsewhere (nutation series, fixed delta-T, lunar radius/limb or ELP frame). Card still not wired to it. Not on-device, no UI change.
- Not mine: #73 conflicting, left alone. Balloons blocked on #314; hams.at key on #400; EME QR link waiting on URL in #440.
- Next: find the 2017/2027 residual (full IAU nutation, delta-T per date), then wire contacts into the Eclipse card.

## 2026-10-19 (UTC), run 54
- Merged #514 (Mac bundle QML, green, other area's). No own PR was open.
- Eclipse residual (D9) investigated, no code change. Experiment in `astro_eclipse_contacts.cpp`: the Sun gets annual aberration (-20.49 arcsec) but the Moon gets none, which is inconsistent for separation at conjunction. Adding the Moon's aberration plus light time shifted every maximum by +50 s (2024 -2 to +48 s, 2027 -59 to -8 s, 2017 -49 to +10 s) and totality 263/384/58 s vs NASA 268/383/160. It improves 2027 but worsens 2024, and the NASA greatest-point maximum time is itself only good to tens of seconds at a rounded station, so the test cannot decide. The 2017 cross-track offset (min separation 27 arcsec) is unchanged either way, so the cause is elsewhere (lunar latitude / ELP-vs-DE offset or nutation). Not adopted; recorded in WIDGETS D9.
- Not mine: #73 left alone. Balloons blocked on #314; hams.at key on #400; EME QR link waiting on URL in #440.
- Next: ELP-vs-DE lunar latitude check against an offline almanac value (Meeus 47.a is too loose); otherwise little unblocked backlog left.

## 2026-10-20 (UTC), run 55
- Issue #529 (Sun & Moon clipped at 1024x600/800x480): the DE/DX value cells, Moon rise/set and Now rows now fill their grid column and fit/elide instead of overflowing; " UTC" suffixes moved into the DE/DX headers ("DE (UTC)"), durations shortened ("10h 24m"), Moon disc capped at 72 px, row-fit estimate taller so "Next season" and later rows drop out rather than overlap. Rendered offscreen at 1024x600 and 800x480 (dark) with a test station: no clipping, no truncation-guard warnings. Not on-device.
- Not mine: #528 CI still running (other area), #73 conflicting. Next: remaining backlog is mostly owner-blocked (#314, #400, #440 URL).

## 2026-10-20 (UTC), run 56
- Balloons pane (backlog D14, first slice): `src/widgets/balloons/` parses SondeHub `/sondes/telemetry?duration=1h` (newest frame per serial; some real frames lack `vel_v`, handled), shows the nearest sonde to DE (distance, compass, type, serial, altitude, climbing/descending, frequency), the next three and a worldwide count, and draws a "balloons" map layer (triangles, max 300, removed when stale). Polled every 10 min, 30 min freshness cut-off. Fixture is a trimmed real response captured today; test `widget-balloons-balloons_regression` (21 checks: parse, phase, nearest/bearing, old-frame drop, live -> stale, map layer). Added `balloons` to PaneChooser's Sky & Space category. Rendered at 800x480, 1024x600 and 1920x1080 in dark and light with no truncation warnings (offscreen; not on-device). Full ctest 167/167.
- Correction to earlier runs: #314 (SondeHub allowlist) and #400 (hams.at: owner said build it) were answered on 2026-10-15; both were wrongly logged as blocked. api.v2.sondehub.org is reachable from the sandbox.
- Next: hams.at API key + personalised passes (D8, #400 decision); balloon landing prediction/track.

## 2026-10-21 (UTC), run 57
- Issue #541: the Balloons card put its empty-state reason in `Metric.caption`, which Metric hides while unavailable. The reason ("Set your station in Settings" / "None heard in the last 30 min") is now a separate Text under the Metric. Rendered offscreen at 1024x600 dark with no station: message visible. Not on-device. The green "Live" badge with no station is the model's status, left as is.
- Not mine: #73 conflicting. Next: hams.at key (D8) per #400; balloon landing prediction.

## 2026-10-21 (UTC), run 58
- hams.at API key (D8, #400): Settings card "hams.at API key" (AppSettings `saveHamsatKey`, stored in the owner-only credentials.ini like the HamAlert password; `hamsatKey()` is C++-only). Shared change, backward compatible: `AuthTransport` / `make_authorized_transport` in `net/polled_source` (bearer header; `make_network_transport` now wraps it) and an optional `WidgetContext::auth_transport` set only for real network runs. HamsatModel sends the key when set, shows workable window / max elevation (`personalised`), refetches when the key changes, and on HTTP 401 drops the key until it changes and shows the public list with an explanation. Cached lists are never shown as personalised. Tests extend `widget-hamsat-hamsat_regression` with a hand-made schema-derived fixture (`upcoming_authed`, marked in its comment): not a live response, no real key was tried. Full ctest 167/167. Not on-device.
- Not mine: #73 conflicting. Next: balloon landing prediction; satellites pass refinements.

## 2026-10-21 (UTC), run 59
- Balloons (D14, slice): descending sondes (>= 2 m/s down) now show "landing in ~N min" in the nearest-sonde detail line. `balloons::landing_eta_s()` integrates the fall under a 1/sqrt(density) speed law in an exponential atmosphere (7.5 km scale height) anchored at the current speed; ignores wind and terrain, labelled as approximate with "~". Test: hand-computed 10 km at 20 m/s = 711 s, plus ground, slow, climbing and unknown cases. Full ctest 167/167 locally; ASan/UBSan left to CI. Text-only change to an existing line, no new layout, so no new screenshots; not on-device.
- Not mine: #73 conflicting, left alone. No `[D]` QA issues open. Next: landing point (needs wind, probably a source decision), satellite pass refinements.

## 2026-10-21 (UTC), run 60
- Issue #553 (QA): the Balloons landing ETA ignored the age of the sonde report, so a 25-minute-old descending report still said "landing in ~12 min". `balloons::landing_eta_remaining_s()` subtracts the report age; when it has elapsed the detail line says "probably landed" instead of a countdown. Test extended in `widget-balloons-balloons_regression` (5 min old: 711 s - 300 s; 25 min old: 0; climbing: unknown). Balloons tests pass locally; full ctest and ASan/UBSan left to CI. Text-only change, no new screenshots; not on-device.
- Not mine: #73 conflicting, left alone. Next: landing point (needs wind, source decision); satellite pass refinements.

## 2026-10-22 (UTC), run 61
- Balloons (D14, slice): `balloons::landing_point()` projects a descending sonde forward by the drift between its last two frames over the remaining fall time; the nearest-sonde detail line adds "lands ~N km <dir> from you". Approximate (no wind profile, overshoots when wind weakens near the ground), labelled with "~". Parser keeps the previous frame for this. Test: hand-computed drift, landed, no previous frame, climbing. Full ctest passes locally; ASan/UBSan left to CI. Text-only change on an existing line, no new screenshots; not on-device.
- Not mine: #73 conflicting, left alone. No `[D]` QA issues open.

## 2026-10-23 (UTC), run 62
- Balloons (D14, slice): the parser keeps each sonde's earlier frames (sorted, thinned to 24) and the model publishes the nearest sonde's flight path as a dotted map layer `balloon-trail` (the overlay API has no polylines; removed when stale or none heard). Test: unordered 40-frame input thins to 24 oldest-first, two frames give one point. Balloons tests pass locally; full ctest and ASan/UBSan left to CI. Map-only change, no card layout change, so no new screenshots; not on-device.
- Not mine: #73 conflicting, left alone. No `[D]` QA issues open. Owner-blocked: #440 (EME link), #475 (ELP tables).

## 2026-10-23 (UTC), run 63
- Issue #572 (Find-in-the-sky, first slice): new plain-C++ `src/domain/astro_finder.{hpp,cpp}` over the SGP4 wrapper: `pointing_at()` (az/el/range/range-rate/sunlit, the 1 Hz feed), `doppler_for()` reusing the existing formulas, `rise_point()`, `describe_pass()` (AOS/TCA/LOS azimuths + compass letters, duration, visible-to-eye = above horizon and sunlit while Sun < -6 deg), `pass_schedule()` with All / VisibleToEye filters. Test `widget-satellites-finder_regression` (21 checks) on the real ISS TLE: AOS/LOS at the cut-off, range rate signs, closest range at TCA, Doppler sense and size, rise point on the horizon, sorting/filter/cap. Honest limits: self-consistency only, not compared with a published pass table; no magnitude estimate. Full ctest 168/168, ASan/UBSan run of the new test clean. No UI yet (schedule pane, Amateur filter, stars, phone layouts remain).
- Not mine: #73 conflicting, left alone.

## 2026-10-23 (UTC), run 64
- Issue #572 (Find-in-the-sky, slice): `pass_schedule()` takes an optional `include(name)` predicate so the schedule pane can apply the Amateur filter (satellites with SatNOGS transmitters) alongside All / Visible-to-eye. Test extended in `widget-satellites-finder_regression` (accept-only, reject-all). Pass az/el tracks already exist as `SatellitePropagator::pass_track`. Satellites/finder ctests pass locally; full ctest and ASan/UBSan left to CI. Domain-only, no UI, not on-device. Remaining for #572: schedule pane UI, starred passes, phone layouts, magnitude estimate.
- Not mine: #73 conflicting, left alone.

## 2026-10-24 (UTC), run 65
- Issue #572 (Find-in-the-sky, slice): new `passes` pane (`src/widgets/satellites/passes_model.*`, `PassesCard.qml`, in the Sky & Space chooser category): next 48 h of passes at DE over `pass_schedule()`, fed by the Satellites model (new `elements()`, `transmitterNorads()` and `elementsChanged` there, so elements download once), All / Amateur / Visible filters saved to `[passes] filter=`. Honest states: no station, waiting for elements, elements older than 30 days. Test `widget-passes-passes_regression` (24 checks, real CelesTrak fixture, five sets). Fixed a QML undefined-row error caught by `qml-pane-passes`. Full ctest passed except the two issues found and fixed here; ASan/UBSan left to CI. Screenshot checked only in the no-station state at 800x480 (offscreen); live rows were not rendered, not on-device.
- Not mine: #73 conflicting, left alone. Remaining for #572: starred passes, magnitude estimate, phone layouts.

## 2026-10-24 (UTC), run 66
- Issue #572 (Find-in-the-sky, slice): Passes pane stars. Tapping a satellite name stars it (saved to `[passes] starred=`), starred rows get a star prefix and a fourth "Starred" filter shows only those, with an explanatory empty state. Test `widget-passes-passes_regression` extended (empty state, star, filter, persistence across a new model, unstar). Passes/satellites ctests pass locally, no build warnings; full ctest and ASan/UBSan left to CI. QML change adds one filter label and tap areas; not re-rendered with live rows, not on-device. Remaining for #572: magnitude estimate, phone layouts.
- Not mine: #73 conflicting, left alone.

## 2026-10-24 (UTC), run 67
- Issue #572 (Find-in-the-sky, slice): estimated visual magnitude for ISS-class passes. `SatellitePropagator::phase_angle_deg()` (small additive API), `estimate_magnitude()` (diffuse-sphere phase law, std mag at 1000 km / 50% phase; only above the horizon, sunlit, Sun below -6 deg) and `standard_magnitude()` (ISS -1.8, Tiangong -1.5; every other satellite gets no magnitude rather than a guess). `pass_schedule()` fills `FinderPass::brightest_mag`; the Passes pane appends " ~-3.1m" to the duration cell. Test extended in `widget-satellites-finder_regression` (known-object gate, plausible range, none in daylight); the range check is trivially true if the fixture epoch gives no dark passes, and it was not compared with a published magnitude table. Accuracy is only about +-1 mag. Finder/passes ctests pass locally; full ctest and ASan/UBSan left to CI. Pane text-only addition, not re-rendered, not on-device. Remaining for #572: phone layouts.
- Not mine: #73 conflicting, left alone.
