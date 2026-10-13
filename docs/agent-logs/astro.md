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
