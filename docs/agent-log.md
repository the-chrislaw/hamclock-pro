# Agent log

## 2026-10-12 (UTC)
- Merged #37 (Qt/CMake-only build) and #38 (release builds) after green CI. Could not delete their head branches (API proxy forbids ref deletion); owner may prune `chore/qt-only-build`, `ci/release-builds`.
- Opened PR `agent/solar-position`: offline solar position library + tests (first slice of the terminator).
- Next: draw night shade + subsolar marker in `world_map_widget`; then DE time zones via QTimeZone.
- Note: `gh pr checks`/GraphQL is unavailable in this environment; used GitHub MCP + REST instead.
