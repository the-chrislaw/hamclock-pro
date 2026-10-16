# Agent log: tvos

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-14 (first run)
- No qa issues for area I; no PRs of mine open.
- Backlog 1: wrote docs/apple-tv.md (architecture, Qt-free shim options, screens, caching, App Store notes, estimate) and the area I section in WIDGETS.md. Opened "Owner input needed: Apple TV design review".
- Next: item 2 (tvos.yml) which does not depend on the design decisions.

## 2026-10-14 (second run)
- Backlog 2: added .github/workflows/tvos.yml (macos-14: host build + tests, tvOS Simulator cross build, tests run via simctl spawn) and a standalone Qt-free cmake/TvOSCore project + cmake/TvOSToolchain.cmake. Shared CMake untouched (no option needed yet). Linux verified natively: 5/5 core tests pass.
- Issue #157 still has no owner reply. Next: item 3 needs the Qt-free SWPC parser port (blocked on #157 answer 1/4); will proceed once answered.

## 2026-10-14 (third run)
- Owner approved all four design recommendations on #157 (compat layer, TV panes, iCloud KVS settings, HCP_CORE_WITHOUT_QT). Design PR and tvos.yml already merged.
- Backlog 3 prep: Qt-free SWPC port in apple/tvos/Core (strict JSON reader + Kp/flux/X-ray parsers, ISO time). Tested on the same real NOAA fixtures as the Qt oracle; added to the standalone TvOSCore build (6/6 pass on Linux). Other areas' parsers untouched, so no shared CMake option was needed yet.
- Next: SwiftUI app target (Space weather + clock) with C bridge to this core and Simulator screenshot artifact.

## 2026-10-14 (fourth run)
- Backlog 3: C ABI (`apple/tvos/Core/hcpcore.h/.cpp`, tested on Linux 7/7), SwiftUI app shell (UTC clock + Kp / solar flux / X-ray cards with Stale/Offline labels, honest User-Agent, NOAA attribution), `project.yml` for XcodeGen, and an `app` job in tvos.yml that builds for the Simulator, launches, and uploads a screenshot artifact.
- Swift/Xcode could not be run in the Linux sandbox; the first tvos.yml run on the PR is the real check and may need follow-up fixes.
- Next: fix any CI findings, then item 4 (panes, map, settings).

## 2026-10-14 (fifth run)
- Main's tvOS CI green (app shell merged as #274). Backlog 4, first slice: Qt-free solar wind parser (`solarwind.cpp`, same fixtures/rules as the Qt oracle), C ABI, and two new cards (solar wind speed, IMF Bz) in the SwiftUI dashboard with Stale/Offline labels. Core tests 8/8 on Linux; Swift verified only by the tvos.yml run on the PR.
- Next: sun/moon card from the shared solar_position code, then satellites/quakes panes.

## 2026-10-15
- Main's tvOS CI green; no open PR of mine, no [I] qa issues. Backlog 4 slice: C ABI for the shared offline solar position (`hcp_subsolar_point`, `hcp_sun_elevation`, tested on Linux 8/8, zero warnings) and a Sun card (subsolar point, computed on device, no Stale state because it needs no network).
- Swift verified only by the tvos.yml run on the PR.
- Next: moon/sun rise-set for the observer once settings (location via iCloud KVS) exist; satellites and quakes panes.

## 2026-10-15 (second run)
- Main's tvOS CI green; no open PR of mine, no [I] qa issues. Backlog 4 slice: station locator. C ABI `hcp_locator_center` over the shared Maidenhead decoder (tested on Linux 8/8, zero warnings), `StationStore` (iCloud key-value store with UserDefaults mirror, per the approved design) and a "Sun at your station" card (on-device elevation, no Stale state) with a locator text field.
- Swift verified only by the tvos.yml run on the PR.
- Next: sun/moon rise-set at the station, satellites and quakes panes.

## 2026-10-15 (third run)
- Main's tvOS CI green; no open PR of mine, no [I] qa issues. Backlog 4 slice: sun/moon rise-set at the station. C ABI `hcp_next_sun_event`, `hcp_next_moon_event`, `hcp_moon_phase` over the Qt-free shared `astro_sun`/`astro_moon` (added to the standalone TvOSCore build; tested on Linux 8/8, zero warnings). Station card now shows next sunrise/sunset, moonrise/moonset (UTC) and Moon phase, all on device, never stale.
- Swift verified only by the tvos.yml run on the PR.
- Next: satellites and quakes panes (need Qt-free TLE / USGS parsers).

## 2026-10-15 (fourth run)
- Main's tvOS CI green; no open PR of mine, no [I] qa issues. Backlog 4 slice: Qt-free USGS quakes parser (`quakes.cpp`, same fixture/rules as the Qt oracle), C ABI (`hcp_quakes_parse`, strongest index, magnitude class), tested on Linux 9/9, zero warnings. New "Earthquakes M2.5+" card (strongest + 5 most recent) with Stale/Offline labels and USGS attribution.
- Swift verified only by the tvos.yml run on the PR.
- Next: satellites pane (needs a Qt-free TLE parser + SGP4 port), settings.

## 2026-10-15 (fifth run)
- Main's tvOS CI green; no open PR of mine, no [I] qa issues. Backlog 4 slice: satellites core. The shared `astro_satellite` (TLE parser + vendored SGP4) is already Qt-free, so it joins the standalone TvOSCore build (SGP4 third-party warnings silenced for that file only). C ABI `hcp_tle_count`, `hcp_tle_name`, `hcp_sat_state` (sub-point, az/el/range, range rate, sunlit, TLE epoch so the UI can flag old elements). Tested on Linux 9/9, zero warnings. No Swift changes this run.
- Next: Swift satellites pane (CelesTrak/AMSAT TLE fetch with Stale label by TLE epoch age; needs allowlist check), settings.

## 2026-10-15 (sixth run)
- Main's tvOS CI green; no open PR of mine, no [I] qa issues. Backlog 4 slice: Swift satellites card. `SatelliteStore` fetches CelesTrak amateur TLEs (<= every 12 h, cached in Caches, honest User-Agent), the shared SGP4 core propagates on device; the card lists the 5 highest satellites above the horizon at the station with az/el/range/eclipse. Labels: STALE ELEMENTS when TLE epoch > 14 days, OFFLINE when the fetch is > 2 days old or absent. CelesTrak attribution added.
- Swift verified only by the tvos.yml run on the PR.
- Next: settings (pane selection), map with shared overlay data.

## 2026-10-15 (seventh run)
- Main's tvOS CI green; no open PR of mine, no [I] qa issues. Backlog 4 slice: pane settings. `PaneStore` (iCloud key-value store + UserDefaults mirror) hides/shows the five panes; a "Panes..." button opens a toggle sheet. Swift only, no core change; Linux build unaffected.
- Swift verified only by the tvos.yml run on the PR.
- Next: map with shared overlay data (needs the shared land/overlay data Qt-free).

## 2026-10-15 (eighth run)
- Main's tvOS CI green; no open PR of mine, no [I] qa issues. Backlog 4 slice: world map. C ABI over the shared Qt-free coastline table (`hcp_coast_part_count`, `hcp_coast_part`, `hcp_coast_points`; tested on Linux 9/9, zero warnings) and a SwiftUI Canvas `MapCard` (coastlines, day/night terminator, subsolar point, station marker; all on device, never stale) plus a "World map" pane toggle. Natural Earth attribution added.
- Swift verified only by the tvos.yml run on the PR.
- Next: map overlays (quakes, satellite sub-points) only while their store is live; DE/DX preset views; land fill.

## 2026-10-16
- Main's tvOS CI green; no open PR of mine, no [I] qa issues. Backlog 4 slice: map overlays. Quakes (USGS, ring radius by magnitude) are drawn only while the feed is live (< 15 min old); satellite sub-points (shared SGP4) only while the TLE fetch is < 2 days old. Both labelled in the map caption. Swift only; core already exposed lat/lon.
- Swift verified only by the tvos.yml run on the PR.
- Next: land fill, DE/DX preset views.
