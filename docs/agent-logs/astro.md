# Agent log: astro

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (UTC)
- Added the NCDXF/IARU beacons pane (backlog D4): `src/domain/astro_beacons.*` implements the published 3-minute / 10-second-slot schedule offline; `src/widgets/beacons/` shows the current beacon on 14.100/18.110/21.150/24.930/28.200 MHz (compact two-column layout). Tests check the schedule rule (4U1UN first slot on 14.100, one band up per slot, wrap, negative time). Beacon locators in the table are approximate and not yet used.
- Not merged by me: #73 (conflicts), #60 (cancelled macOS checks), #84 (CI pending) - other areas.
- Next: beacon map markers once area G's overlay API exists; then satellites (D3: vendor SGP4 + licence note in docs/).
