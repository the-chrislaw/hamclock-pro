# Agent log: hazards

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 21:00 UTC
- Merged #75 (docs: scale agents). Left #73 (merge conflicts, other area) and #60 (macOS jobs cancelled) alone.
- H1 slice 1: earthquakes pane (`src/widgets/quakes`): USGS 2.5_day GeoJSON parser, model (strongest, count, recent), card with compact layout. Fixture is a real capture from earthquake.usgs.gov (reachable from the sandbox), trimmed to 8 events. Terms recorded in the roadmap's Data source policy. Screenshots checked at 800x480, 1024x600, 1920x1080, light and dark; ASan/UBSan test run clean.
- Next: map overlay for quakes once area G's overlay API exists; otherwise H2 tropical storms (NHC).
- Blocked: nothing. No hazard samples exist in tests/fixtures/sources; USGS fetched directly.

## 2026-10-13 22:00 UTC
- Step 1: no PRs of mine open. #87/#88 (other areas) still had CI running, #73 has merge conflicts; left alone.
- H2 slice 1: tropical storms pane (`src/widgets/storms`): NHC CurrentStorms.json parser, model (headline storm, category, movement, issuance time, up to 3 others), compact card. Fixture is a real capture (2026-10-13, two East Pacific storms) trimmed to the fields used. Terms recorded in the roadmap's Data source policy. Screenshots checked at 800x480, 1024x600, 1920x1080, light and dark (first layout clipped the headline; fixed). ASan/UBSan run clean. JTWC (West Pacific) and the track overlay are not done.
- Next: H3 NWS marine and fire-weather warnings (api.weather.gov reachable), then quakes/storms overlays once area G's API lands.
- Blocked: nothing.

## 2026-10-13 23:00 UTC
- Step 1: merged #98 (docs, green). #73 still has merge conflicts (other area), #96/#99 had CI running; left alone. No PRs of mine open.
- H3 slice 1: NWS marine warnings pane (`src/widgets/marine`): api.weather.gov alerts parser, model (worst alert, area, issued/until, counts by type; expired alerts dropped by clock), compact card. Fixture is a real capture (2026-10-13 22:52Z) trimmed to 6 alerts and named `active` to match the fixture transport. Terms recorded in the roadmap. Screenshots checked at 800x480, 1024x600, 1920x1080 light/dark; ASan/UBSan run clean.
- Next: H3 slice 2: fire-weather (Red Flag Warning) pane in `firewx`, then marine region filter; then lightning.
- Blocked: nothing.

## 2026-10-14 00:00 UTC
- Step 1: #107 (area G overlay API) had macOS/Pi jobs still running, #73 mergeable state unknown; left alone. No PRs of mine open. No `qa` issues for [H].
- H3 slice 2: fire-weather pane (`src/widgets/firewx`): same api.weather.gov alerts feed (Red Flag Warning, Fire Weather Watch, Fire Warning, Extreme Fire Danger), model with short headline for compact cards, "starts ..." line for future watches, issued/until times, expired alerts dropped by clock. Fixture is a real capture (2026-10-13 23:51Z); no Red Flag Warning was active then, so it holds one Fire Weather Watch (no invented data; Red Flag handled by rank/sort unit checks only). Screenshots checked at 800x480, 1024x600, 1920x1080 light/dark (fixed headline clip and caption fall-through). ASan/UBSan run clean.
- Note: marine and firewx fixtures are both named `active` (URL path), so screenshot runs need separate `--fixtures` dirs.
- Next: marine region filter, then lightning (blocked on a terms-checked source), quakes/storms overlays once #107 merges.
- Blocked: nothing.

## 2026-10-14 01:00 UTC
- Step 1: #115 (G3) and #73 still had CI running / conflicts, other areas, left alone. No PRs of mine open; no `qa` issues for [H].
- H1 slice 2: quakes map overlay via area G's API (`quakes_overlay.*`): one dot per quake, strongest first, size and colour by magnitude (yellow < M4.5, orange < M6, red), "M6.8" labels, capped at 60; layer removed when data is not live or map visibility is off (`setMapVisible`). Tests cover order, size, cap, off/on, stale removal. Pane unchanged, so no new screenshots needed; overlay rendering itself is area G's.
- Next: storms track overlay, then lightning (needs a terms-checked source), fires (FIRMS key via settings), nets.
- Blocked: nothing.

## 2026-10-14 02:00 UTC
- Step 1: #131 (area G) and #73 (other area) not green yet / conflicting; left alone. No PRs of mine open; no `qa` issues for [H].
- H2 slice 2: storms map overlay (`storms_overlay.*`): one diamond per storm at its NHC position, strongest first, name label, colour by category (yellow < cat 1, orange cat 1-2, red cat 3+), `setMapVisible`, layer removed when data is not live. Tests cover order, colour, position, size, off/on, stale removal; debug and ASan/UBSan suites pass. Pane unchanged so no new screenshots. Forecast tracks need cone/track data (NHC GIS/KMZ) and are not done.
- Next: marine region filter, nets (NetLogger), lightning (needs a terms-checked source), fires (FIRMS key via settings).
- Blocked: nothing.

## 2026-10-14 04:00 UTC
- Step 1: no PRs of mine open. #161 (area G) had CI running, #73 (other area) left alone. No `qa` issues for [H]. #142 (allowlist: NetLogger, lightning, JTWC) still open; api.netlogger.org and data.blitzortung.org unreachable, metoc.navy.mil gives 403.
- H4 slice 1: active fires (`src/widgets/fires`): NASA FIRMS keyless regional VIIRS CSV (US + Hawaii, 24 h, ~110 KB), parser by header column, model (count, high-confidence count, strongest FRP, newest overpass), compact card, map overlay (60 strongest non-low-confidence, triangles, red >= 50 MW). Fixture is 40 rows trimmed from a real capture (2026-10-14). Terms recorded in the roadmap. Screenshots checked at 800x480, 1024x600, 1920x1080 light/dark (detail line shown at all sizes). ASan/UBSan run.
- Next: marine region filter; lightning and nets once #142 is resolved; fires near DE once the station is reachable from the widget context.
- Blocked: lightning and NetLogger on allowlist (#142).

## 2026-10-14 05:00 UTC
- Step 1: merged #168 (F3, green). #169 (G) had CI running, #73 (other area) left alone. No PRs of mine open; no `qa` issues for [H]. #142 allowlist still open: api.netlogger.org, data.blitzortung.org, metoc.navy.mil still unreachable, so lightning, nets and JTWC stay blocked.
- H1 slice 3: quakes pane shows "Nearest M4.1 · 820 km NW of DE · 3 h ago" using the saved DE station (great-circle distance, 8-point bearing); hidden until a station is configured and refreshed when the station changes. Tests cover no-station and with-station. Debug and ASan/UBSan quakes tests pass. The quakes card is not on the default dashboard, so I could not screenshot the new row at 800x480, 1024x600, 1920x1080; it is a single elided text line. Not validated visually.
- Next: marine/firewx filter by DE, fires near DE; lightning and nets once #142 is resolved.
- Blocked: lightning and NetLogger on allowlist (#142).

## 2026-10-14 06:00 UTC
- Step 1: merged #177 (F4, green). #73 (other area) has merge conflicts, #178 CI still running; left alone. No `qa` issues for [H]. #142 allowlist still open; lightning, nets, JTWC stay blocked.
- H3 slice 3: fire-weather pane shows "At DE: Fire Watch · until 17 05:00Z" / "At DE: none active" via api.weather.gov `alerts/active?point=` for the saved station (second PolledSource, rebuilt on station change; hidden without a station, hidden when the point data is stale so it never reads as "none active"). Marine not done: its zones are offshore, so a point query does not apply; a marine region filter needs zone geometry and is still open. Fixture `point-empty.json` is a real capture (Denver, 2026-10-14 05:45Z, nothing active); the non-empty case reuses the real `active` body. Screenshots checked at 800x480, 1024x600, 1920x1080 (shortened event name and 2-line wrap fixed a clipped "until"). Debug suite 109/109 and ASan/UBSan firewx test pass.
- Next: marine region filter (needs zones), fires near DE; lightning and nets once #142 is resolved.
- Blocked: lightning and NetLogger on allowlist (#142).

## 2026-10-14 07:00 UTC
- Step 1: merged #185 (E6, required checks green). #186, #187, #73 are other areas (Apple jobs still running / #73 conflicts); left alone. No PRs of mine open; no `qa` issues for [H]. #142 allowlist still open: api.netlogger.org and data.blitzortung.org unreachable, metoc.navy.mil 403; lightning, nets, JTWC stay blocked.
- H3 marine region slice: marine pane shows "Gulf of Mexico: 2 · Gale Warning until 14 04:00Z" / "…: none active" for the NWS marine region nearest the saved DE (`alerts/active?region=GM|AT|PA|AL|PI|GL`). Region chosen from coarse coastline boxes (`marine::region_for`), empty inland so the line is hidden; hidden when regional data is stale. Fixture `region-empty.json` is a real GM capture (2026-10-14 06:52Z, nothing active); the non-empty case reuses the real `active` body. Tests cover region mapping, no-station, empty, alerts, stale. Coastline boxes are approximate (e.g. Florida east/west coast, Great Lakes edge), not geometry.
- Next: fires near DE; marine map overlay needs zone geometry; lightning and nets once #142 is resolved.
- Blocked: lightning and NetLogger on allowlist (#142).

## 2026-10-14 08:00 UTC
- Step 1: no PRs of mine open; #194, #186, #73 are other areas, left alone. No `qa` issues for [H]. #142 allowlist still open; lightning, nets, JTWC stay blocked.
- H4 slice 2: fires card shows "Nearest 320 km NW of DE · 13 18:38Z" (nearest non-low-confidence detection to the saved station; hidden without a station, refreshed on station change). Tests cover no-station and with-station. Full suite 118/118 and ASan/UBSan fires test pass. Fires is not on the default dashboard, so the new single elided line was not screenshotted at 800x480/1024x600/1920x1080 (full-app screenshots render, but without the card); not validated visually.
- Next: marine map overlay (needs zone geometry); lightning and nets once #142 is resolved.
- Blocked: lightning and NetLogger on allowlist (#142).

## 2026-10-14 10:00 UTC
- Step 1: no PRs of mine open; #207 (no checks yet) and #211 (CI running) are other areas, left alone. #142 allowlist still open; lightning, nets, JTWC stay blocked.
- QA #203: quakes and fires "Nearest ..." lines were elided at 800x480/1024x600. Both now wrap to up to 2 lines; the quakes strongest-place text moved out of the Metric caption into its own wrapping line. QML only, no model change. Rendered quakes and fires with a saved station at 800x480, 1024x600, 1920x1080 light/dark: no TRUNCATED output, nothing clipped; quakes/fires tests pass. Not validated on a device.
- Next: marine map overlay (needs zone geometry); lightning and nets once #142 is resolved.
- Blocked: lightning and NetLogger on allowlist (#142).

## 2026-10-14 12:00 UTC
- Step 1: merged #221 (F4 iPad CI, all five checks green). #73 (other area) has merge conflicts, #220 had no checks yet; left alone. No `qa` issues for [H]. #142 allowlist: api.netlogger.org and data.blitzortung.org still unreachable (netlogger.org site itself answers but has no documented API), metoc.navy.mil 403; lightning, nets and JTWC stay blocked.
- H2 slice 3: storms forecast positions on the map. Parses the NHC forecast/advisory text (`parse_forecast`: six forecast and two outlook points with valid time and wind; day/month resolved from the issuance), one PolledSource per storm for the three strongest. Dots (3-4 px, coloured by forecast intensity, tap shows "Trudy forecast 14 18:00Z · 55 kt") are added after the storm markers; used only when the text's storm id and advisory number match the index and the page is fresh, otherwise not drawn. The overlay API has no lines, so no connecting track or cone; the KMZ track needs a zip reader. Fixtures: real forecast advisory and a trimmed real index (Trudy, advisory 4, captured 2026-10-14). Full suite 132/132; ASan/UBSan storms test run. Pane unchanged, so no screenshots.
- Next: marine map overlay (needs zone geometry); lightning and nets once #142 is resolved.
- Blocked: lightning and NetLogger on allowlist (#142).

## 2026-10-15 (hourly run)
- Step 1: open PRs #358, #357, #73 are other areas; left alone. No `qa` issues for [H].
- Owner decision on #142 (2026-10-15): skip NetLogger (TLS host mismatch) and lightning (GLM too heavy). Added both to ROADMAP "On the radar". JTWC (metoc.navy.mil still 403) and the marine polygon overlay (overlay API has no polygons) remain blocked. Area H has no buildable backlog left. Docs only; no code or screenshots.
