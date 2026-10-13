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
