# Agent log: ui

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (hourly run)
- Issue #80: added `--pane N=<widget id>` (repeatable) to choose pane contents for screenshots; unknown ids exit 2. CMake registers one `qml-pane-<id>` test per widget (800x480, fails on any QML warning), so every widget is now render-checked in CI.
- Not merged this run: #73 (docs, conflicts with main; not an agent/ui branch), #60 (area F, macOS/iPad checks cancelled).
- Next: A0 remainder, starting with removing the header bar on small screens and tight spacing at 1024x600, then the `compact` mechanism check in each card.

## 2026-10-13 (hourly run, 2)
- Merged #89 (docs, area G priorities). #73 still has merge conflicts and is not an agent/ui branch; left alone.
- `Theme.dense` (window <= 1280x720): no header bar, pad/gap/radius roughly halved, narrower station column, station clock fits its column, Settings button moved into the station card. Tap anywhere on a pane opens the chooser; horizontal swipe steps through widgets (including empty). Checked by screenshots at 800x480, 1024x600, 1920x1080; ctest 64/64. Not validated on a device.
- Next: long-press widget options, per-slot rotation, per-widget `compact` audit.
