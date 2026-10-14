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
