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

## 2026-10-13 (hourly run, 3)
- Merged #99 (area G map core, CI green). #100 (area H) still had the iPad check running; #73 was already stale.
- Long-press on a pane opens `PaneOptions` (change widget, next, previous, empty, reset all panes). ctest 71/71; screenshots rendered at 1024x600 and 800x480. ASan/UBSan left to CI (QML-only change). Not validated on a device.
- Next: per-slot rotation, per-widget `compact` audit.

## 2026-10-14 (hourly run)
- Merged #108 (area H, CI green). #107 (area G) still had Pi package builds running; #73 is stale with conflicts and not an agent/ui branch.
- Per-slot pane rotation: `AppSettings.paneRotations` / `setPaneRotation` (off, 5, 10, 30, 60 s; saved in options.ini, cleared by reset), a `PaneSlot` timer cycles through all widgets without writing settings, pauses while a menu is open; long-press menu has a "Rotate" button. ctest 80/80; screenshot 1024x600 unchanged. ASan/UBSan left to CI. Not validated on a device.
- Next: custom rotation sets per slot (category picker, A8), per-widget `compact` audit.

## 2026-10-14 (hourly run, 2)
- Issue #119: `Card` footer is now a status Text (elides, two lines in non-compact slots) plus a separate age Text that never elides, so the data age is always visible. ctest 82/82; screenshots 800x480 and 1024x600 checked (no fixtures for age, so only the status line shows). ASan/UBSan left to CI. Not validated on a device.
- #124 (QA docs) had CI still running at start; #73 stale. Next: custom rotation sets per slot (A8), per-widget `compact` audit.
