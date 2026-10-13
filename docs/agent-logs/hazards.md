# Agent log: hazards

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 21:00 UTC
- Merged #75 (docs: scale agents). Left #73 (merge conflicts, other area) and #60 (macOS jobs cancelled) alone.
- H1 slice 1: earthquakes pane (`src/widgets/quakes`): USGS 2.5_day GeoJSON parser, model (strongest, count, recent), card with compact layout. Fixture is a real capture from earthquake.usgs.gov (reachable from the sandbox), trimmed to 8 events. Terms recorded in the roadmap's Data source policy. Screenshots checked at 800x480, 1024x600, 1920x1080, light and dark; ASan/UBSan test run clean.
- Next: map overlay for quakes once area G's overlay API exists; otherwise H2 tropical storms (NHC).
- Blocked: nothing. No hazard samples exist in tests/fixtures/sources; USGS fetched directly.
