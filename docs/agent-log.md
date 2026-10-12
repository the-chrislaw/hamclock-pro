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
