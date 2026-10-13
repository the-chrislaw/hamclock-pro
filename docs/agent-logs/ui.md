# Agent log: ui

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (hourly run)
- Issue #80: added `--pane N=<widget id>` (repeatable) to choose pane contents for screenshots; unknown ids exit 2. CMake registers one `qml-pane-<id>` test per widget (800x480, fails on any QML warning), so every widget is now render-checked in CI.
- Not merged this run: #73 (docs, conflicts with main; not an agent/ui branch), #60 (area F, macOS/iPad checks cancelled).
- Next: A0 remainder, starting with removing the header bar on small screens and tight spacing at 1024x600, then the `compact` mechanism check in each card.
