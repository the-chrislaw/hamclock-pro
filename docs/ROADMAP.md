# HamClock Pro roadmap

Written October 12, 2026. This is the single plan from today's state to a 1.0 release. It replaces no evidence document; it orders the work. Detailed specs, audits and validation notes stay in `docs/` and `docs/workstreams/`.

## Where we are

- **Code:** C++17, Qt 6 dashboard plus an earlier SDL2 experiment. 30 CTest suites pass locally on macOS; `make verify` passes.
- **Branches:** `main` holds only the README and license. All work is in 35 open draft PRs stacked one on another (#1 → #35), plus `wip/de-clock-settings-dialog` on top of #35. Nothing is merged.
- **CI:** none. Tests only run on whichever machine someone builds on.
- **Hardware:** never run on a Raspberry Pi.
- **Design:** the approved look is `design/dashboard.html` and the refined mockups; the native Qt dashboard does not match it yet (`docs/native-approved-design-gap.md`).

## Definition of 1.0

A Pi 4 with the official 7-inch touchscreen boots straight into HamClock Pro and shows, with real data and no simulated readings:

- UTC and DE local time, DE/DX stations with locator, distance and bearing
- the world map with land fill, day/night terminator and the DE→DX path
- space weather (Kp, solar flux, X-ray) and a working DX cluster spot list
- a settings screen that saves and restores everything above
- the approved visual design in light and dark, usable by touch alone

It must also install and update with one documented command, and run acceptably on a Pi 3. Everything else in original HamClock 4.22 is post-1.0.

## Milestones

Sizes are relative effort: S = days, M = 1–2 weeks, L = several weeks. They assume one main developer with AI agents.

### M0: Consolidate (S), do first

Get one healthy `main` before writing more features.

1. Review the stack bottom-up and merge #1 → #35 in order, then the `wip/` branch. Squash-merge is fine; keep each PR's title as the commit.
2. Add GitHub Actions: Ubuntu + Qt 6 build, `ctest`, and an ASan/UBSan job. Add an arm64 build job to catch Pi compile problems early.
3. Delete merged branches. From now on, feature PRs target `main` directly; no stacks more than 2 deep.
4. Close or merge the Codex workspace's uncommitted copy so there's one source of truth (see "Working agreements").

**Done when:** `main` builds green in CI with all 30 suites, and there are no open stacked drafts.

### M1: Finish the station clock (M)

Turn the pieces that already exist into one working screen.

- Wire the DE clock settings dialog into Big Clock → Options. Apply the two corrections in `docs/workstreams/de-native-integration-plan.md`. Fix the disabled offset field not looking disabled.
- DE local time with real time zones and daylight saving (P1 in `docs/parity-next.md`). Use the OS tz database through Qt's `QTimeZone`; don't hand-roll DST rules.
- Day/night terminator and subsolar point on the map (computed offline; no data feed needed).
- Retire the SDL2 executable, or mark it unmaintained, so there's one UI to keep working.

**Done when:** someone can set their station, see correct UTC and local time across a DST change, and watch the terminator move, on a Mac.

### M2: Approved design in native Qt (M)

Follow `docs/native-approved-design-gap.md`: station-first left card, three instrument panes plus space-weather strip, rounded map card, semantic light/dark tokens, fixed type scale. Panes without data show a clear "not available yet" state.

**Done when:** side-by-side screenshots of native vs. the approved mockup at 800×480 and 1280×800 are signed off by the owner.

### M3: First run on a Pi 4 (S–M)

- Build on Raspberry Pi OS (Bookworm, 64-bit), Qt 6 from distro packages.
- Fullscreen kiosk launch at boot (systemd user service or autostart).
- Measure CPU, memory, startup time and frame rate; record them in `docs/`.
- Touch-only pass: every control reachable and at least 44 px; decide whether an on-screen keyboard is needed for callsign entry.

**Done when:** the owner uses it on the Pi 4 touchscreen for a day without a crash or needing a keyboard.

### M4: Live data (L)

Each feed needs a recorded decision on source, terms of use and polling rate *before* coding (see "Decisions needed").

1. **Space weather:** NOAA SWPC JSON for Kp (parser exists), solar flux, X-ray. Cache to disk; show age and an offline state.
2. **DX cluster:** telnet client to a user-chosen node with callsign login; feed the existing `spot_store`. Plot spots on the map.
3. **Data plumbing shared by both:** one fetch scheduler, backoff, timeouts, and stale/error/offline states in the UI.

**Done when:** a 24-hour soak test on the Pi with Wi-Fi drops shows correct recovery and no memory growth.

### M5: 1.0 release (M)

- Install/update script or `.deb`; uninstall leaves no system changes.
- Pi 3 measurement pass; fix or document limits.
- Third-party notices (Qt LGPL, Natural Earth, data sources). Settle the project name (does "HamClock" in the name need permission?).
- User guide: install, first-run setup, troubleshooting.
- Tag `v1.0.0`, publish the repo publicly.

## After 1.0

Rough order, highest value first. Each item is one PR-sized feature or a short series.

| Order | Feature (4.22 family) | Notes |
|---|---|---|
| 1 | Sun & Moon panes (F04) | Offline astronomy; no data permissions needed |
| 2 | Live spots via PSKReporter (F14) | Check PSKReporter usage terms |
| 3 | Satellite predictions (F05) | Needs TLE source (CelesTrak) and SGP4 library choice |
| 4 | Band conditions / propagation (F07) | Online VOACAP needs prior agreement; consider an offline model |
| 5 | NCDXF beacons (F19) | Offline schedule |
| 6 | Map projections, grid/zones/cities overlays (F11, F12) | Data licensing per dataset |
| 7 | Contests & DXpeditions (F17), On-the-air (F18) | Feed terms per source |
| 8 | DE/DX weather (F10), solar imagery (F09) | API keys / image terms |
| 9 | Radio & rotator control (F20) | Hamlib; requires safety review |
| 10 | GPS (F21), sensors & CPU temp (F22) | Pi hardware specific |
| 11 | Brightness & display schedule (F27) | Pi display backlight control |
| 12 | Browser view & REST API (F28) | Security review required before any network listener |
| 13 | ADIF import (F15), watch lists (F16), news feeds (F24) | |
| 14 | Saved configurations, pane rotation (F25, F26), diagnostics/updates (F29) | |
| 15 | EME planning (F06) | |

## Status by feature family

From the F01–F30 inventory in Codex's `outputs/hamclock-pro-feature-audit.md`. "Partial" means some code and tests exist, not that it matches 4.22.

| Family | Status | Milestone |
|---|---|---|
| F01 Callsign, station coordinates | Partial: editor, validation, saved settings | M1 |
| F02 Clocks, time zones | Partial: UTC, Big Clock, stopwatch, countdown, manual DE offset | M1 |
| F03 Maidenhead locator | Partial: encode/decode with boundary tests | M1 |
| F04 Sun, Moon | Not started | After 1.0 |
| F05 Satellites | Not started | After 1.0 |
| F06 EME | Not started | After 1.0 |
| F07 Propagation, bands | Not started | After 1.0 |
| F08 Space weather | Partial: offline Kp parser only | M4 |
| F09 Solar imagery | Not started | After 1.0 |
| F10 DE/DX weather | Not started | After 1.0 |
| F11 Maps, grayline | Partial: coastlines, land fill, great-circle path; no terminator | M1 |
| F12 Zones, cities, prefixes | Not started | After 1.0 |
| F13 DX cluster | Spot model only | M4 |
| F14 Live spots | Spot model only | After 1.0 |
| F15 ADIF | Not started | After 1.0 |
| F16 Watch lists | Not started | After 1.0 |
| F17 Contests, DXpeditions | Not started | After 1.0 |
| F18 On-the-air | Not started | After 1.0 |
| F19 Beacons | Not started | After 1.0 |
| F20 Radio, rotator | Not started | After 1.0 |
| F21 GPS | Not started | After 1.0 |
| F22 Sensors | Not started | After 1.0 |
| F23 Timers, alarms | Partial: stopwatch and countdown; no alarm sound | M1 (as is) |
| F24 News | Not started | After 1.0 |
| F25 Setup, saved configs | Partial: single station file | M1 / M4 |
| F26 Pane selection | Partial: keyboard pane focus; instrument catalog | M2 |
| F27 Brightness | Not started | After 1.0 |
| F28 Browser, API | Not started | After 1.0 |
| F29 Diagnostics, updates | Not started | M5 (update only) |
| F30 Input, menus, tooltips | Partial: keyboard focus, 44 px targets | M2 / M3 |

## Decisions needed from the owner

These block the milestone shown. Each needs a short written answer in this file or an issue.

1. **M0:** OK to merge the stacked drafts into `main` as-is after a quick review, rather than re-auditing each one?
2. **M1:** Drop the SDL2 build and go Qt-only?
3. **M3:** Which Pi OS image and display exactly (official 7" v1 or v2, or another)?
4. **M4:** Data sources for 1.0: NOAA SWPC for space weather? Which DX cluster node by default, or user must enter one?
5. **M5:** Project name and whether to ask the original HamClock author before using "HamClock".
6. **M5:** Licence stays MIT? Public repo at 1.0, or earlier?

## Working agreements

- **One writer at a time.** Either Codex or Claude Code works in a given branch, never both. The Codex checkout at `~/Documents/Codex/2026-10-02/…/hamclock-pro` still has uncommitted copies of the DE settings work; retire it or reset it to `main` after M0.
- **Lighter process per feature:** one short spec section, code, tests, one independent review, and an on-screen check for UI changes. Put findings in the PR description instead of separate audit files. Keep `docs/overnight-checkpoint.md` as a short log, not the place where decisions live.
- **PRs target `main`** and stay small; CI must be green before merge.
- **No simulated data shown as live.** Unavailable panes say so.
- **Hardware claims need hardware.** "Works on Pi X" only after a recorded run on Pi X.
