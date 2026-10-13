# Agent log

## 2026-10-12 (UTC)
- Merged #37 (Qt/CMake-only build) and #38 (release builds) after green CI. Could not delete their head branches (API proxy forbids ref deletion); owner may prune `chore/qt-only-build`, `ci/release-builds`.
- Opened PR `agent/solar-position`: offline solar position library + tests (first slice of the terminator).
- Next: draw night shade + subsolar marker in `world_map_widget`; then DE time zones via QTimeZone.
- Note: `gh pr checks`/GraphQL is unavailable in this environment; used GitHub MCP + REST instead.

## 2026-10-12 (UTC), run 2
- #39 (solar position) was already merged; no open PRs.
- Opened `agent/map-terminator`: night shade + subsolar marker in `WorldMapWidget`, wired to the dashboard clock (repaints once per UTC minute). Dashboard regression relaxed only to allow that minute-rollover repaint.
- Next: DE local time via QTimeZone; DE settings dialog from Big Clock -> Options.

## 2026-10-13 (UTC)
- Merged #41 (owner decisions: platforms, data sources, name, licence) after green CI.
- Opened `agent/de-timezone`: `resolve_de_zone_offset` / `is_valid_de_zone_id` over QTimeZone + DST regression test (first slice of DE local time).
- Next: wire zone offset into DE Automatic civil adapter and settings dialog (zone id field); then DE settings dialog from Big Clock -> Options.

## 2026-10-13 (UTC), run 2
- No open PRs or issues. #42 (DE zone offset) already merged.
- Opened `agent/de-auto-zone`: `adapt_de_clock_zone_civil` (IANA zone + UTC -> DE civil time) + DST regression test.
- Note: existing -Wall warnings in de_clock_settings_dialog and two tests are pre-existing.
- Next: add zone id to DE configuration/Automatic mode and settings dialog; then DE settings dialog from Big Clock -> Options.

## 2026-10-13 (UTC), run 3
- No open PRs or issues; #43 already merged.
- Opened `agent/de-zone-config`: optional IANA zone id in `DeClockConfiguration` (fixed array, allocation-free) + display snapshot uses `adapt_de_clock_zone_civil` for Automatic+zone; unknown zone stays unresolved.
- Next: zone id field in DE settings dialog; DE dialog from Big Clock -> Options; then go-public prep.

## 2026-10-13 (UTC), run 4
- No open PRs or issues; #44 already merged.
- Opened `agent/de-dialog-zone`: IANA zone id field in the DE settings dialog (Automatic mode, validated against the tz database) + regression tests; rendered offscreen at 800x480..1920x1080 light/dark.
- Next: persist DE configuration; DE dialog from Big Clock -> Options; then go-public prep.

## 2026-10-13 (UTC), run 5
- No open PRs or issues; #45 already merged.
- Opened `agent/de-persist`: optional `de_zone` in the station settings file (legacy files still load; bad ids rejected), station editor preserves it.
- Next: feed saved zone into the dashboard DE configuration; DE dialog from Big Clock -> Options; then go-public prep.

## 2026-10-13 (UTC), run 6
- Merged #47 (Trixie/Bookworm x arm64/armhf packages) and #48 (qt6-wayland dependency, first Pi 4 run) after green CI.
- Opened `agent/de-load-zone`: `de_clock_configuration_for_saved_zone` (saved `de_zone` -> startup DE configuration) + tests.
- Next: dashboard shows DE local time from it; DE dialog from Big Clock -> Options; then go-public prep.

## 2026-10-13 (UTC), Spots (area C)
- Opened `agent/spots-dxcluster`: `src/net/telnet_client` (injected socket, IAC refusal, login prompt, backoff), DX spot parser, DX cluster model + card (Not configured without host/callsign). Fixture is a hand-trimmed sample in DXSpider format, not a live capture.
- Next: spots on the map, band/mode filters; then PSKReporter.

## 2026-10-13 (UTC), Astro (area D)
- No open PRs. Opened `agent/astro-sunrise`: `src/domain/astro_sun` (NOAA/Meeus sunrise/sunset, polar day/night) validated against published London solstice times and the existing Meeus-tested solar position; `sunmoon` widget showing next sunrise/sunset and day length at DE and DX, offline.
- Next: moon phase, rise/set, az/el at DE (same widget); then SGP4 satellites.

## 2026-10-13 (UTC), Weather (area E)
- Opened `agent/wx-openmeteo`: `src/widgets/weather` (Open-Meteo current conditions at DE and DX, strict parser, response-location check so a stale cache for an old station is never shown, unconfigured/stale/offline states). Fixture is the real response captured 2026-10-13 from #53's sample set.
- #53 (not mine) was still in CI; left alone.
- Next: callsign lookup (QRZ/HamQTH) session handling.

## 2026-10-13 (UTC), Space weather (area B)
- No open PRs. Opened `agent/space-solarwind`: `src/widgets/solarwind` (SWPC RTSW wind + mag 1-min feeds; active spacecraft only, null/-9999 fill rejected; 1 h sparklines; stale when the newest observation is >15 min old even if the download is fresh). Fixtures are trimmed real captures from 2026-10-13. Same SWPC policy as the existing widget, polled every 2 min.
- Next: sunspot number + 30-day flux trend chart, Kp 3-day bars.

## 2026-10-13 (UTC), UI shell (area A)
- Merged #55 (docs, green). #57 was still in CI.
- Opened `agent/ui-scroll`: with four widgets the wide layout clipped the cards and pushed the world map off-screen with no way to reach it; the page now scrolls in wide mode too and the map keeps a 2:1 size. Checked 800x480 offscreen screenshot only; map is below the fold there.
- Next: equal-height widget rows (gap under Space weather), pressed/focus states, TV focus order.

## 2026-10-13 (UTC), Space weather (area B), run 2
- Left #60 (area F, macOS/iPad checks cancelled) alone. Opened `agent/space-solartrend`: `src/widgets/solartrend` (3-day Kp bars with faded forecast, 30-day daily Noon 10.7 cm flux trend, latest NOAA monthly sunspot number). Fixtures are trimmed real captures from 2026-10-13 (`tests/fixtures/sources`). Same SWPC policy; no new sources or hosts.
- Checked offscreen light/dark at 800x480, 1024x600, 1920x1080 (below the fold in the first two by the shell's layout) plus a tall window; ASan/UBSan test run clean.
- Next: band conditions (F07) from SFI/Kp/A.

## 2026-10-13 (UTC), Spots (area C), run 2
- Opened `agent/spots-filters`: band and mode filters for the DX cluster list (model properties, tap-to-cycle buttons on the card, "No spots match" state) + tests. The card layout was also fixed to stack in a column. Offscreen screenshots cannot show the spot list (no cluster in fixtures), so the filter row is checked by model tests only; map overlay is area A's map, still pending.
- Next: PSKReporter (F14); spots on the map once the QML map exposes an overlay API.

## 2026-10-13 (UTC), Astro (area D), run 2
- No open PRs of mine (sunrise merged). Opened `agent/astro-moon`: `src/domain/astro_moon` (Meeus ch. 47 principal terms, phase/illumination, topocentric az/el, rise/set) validated against Meeus example 47.a and almanac new/full/quarter instants (elongation within 0.01-0.4 deg); Sun & Moon card gains phase, moonrise/set and az/el at DE. All offline.
- Next: SGP4 satellites (vendor library + licence in docs/, CelesTrak TLE cache, passes at DE).
