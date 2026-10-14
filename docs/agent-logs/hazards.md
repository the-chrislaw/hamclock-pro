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
