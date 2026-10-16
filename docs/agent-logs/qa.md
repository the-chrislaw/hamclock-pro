# Agent log: qa

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (UTC) QA run 1
- Last reviewed commit: c63baa195b54e7c4b81a25c4db6fac89eafca55e (PRs #44 to #76, last 24 h).
- Build and 48/48 tests pass on main. Rendered 800x480, 1024x600 and 1920x1080, light and dark, with all widget fixtures; default panes only (see #80).
- Opened: #78 [B] Space weather X-ray and Solar wind Bz clipped; #79 [G] map letterboxed at wide sizes; #80 [A] no way to screenshot non-default panes. Closed: none.
- Code scan of lookup (#66) found no password exposure to QML or logs, no QtWidgets in widgets, no http:// URLs. Widget diffs were not reviewed line by line this run.
- Health: one-page layout works with no scrolling, but compact-pane clipping remains in the Space weather and Solar wind cards (already seen on the real Pi). The Bands, Lookup, Weather, Sun/Moon and Solar trend panes are not yet reviewable visually.

## 2026-10-14 (UTC) QA run 2
- Last reviewed commit: a695f6555546b0db1f3e77535d084afe1a90e972 (PRs #77 to #115, 40 merges since c63baa1).
- Build and 82/82 tests pass on main. Rendered every pane widget (watchlist is not a pane) at 800x480 and 1024x600 (light and dark), default panes at 1920x1080. Closed since last run: #78, #79, #80 (closed by their fixers; the clipped Space weather/Solar wind cards, map fill and `--pane` option are confirmed fixed on main).
- Opened: #117 [E] weather DE/DX columns overlap; #118 [D] Sun & Moon Moonset row sliced at 1024x600; #119 [A] card footer elides the data age; #120 [B] X-ray plot axis letters overprint at 1920x1080; #121 [B] Solar trend labels truncated at 800x480; #122 [G] map labels collide / do not scale; #123 [F] Linux credential store is plain JSON (0600).
- Diff scan of the 40 PRs: no QtWidgets in widgets, no http:// URLs, no scrolling views, no passwords in Q_PROPERTY or logs, no weakened test assertions. Per-widget code was not read line by line.
- Health: one-page layout is solid at all sizes and the earlier clipping is fixed; the remaining problems are small text-fit issues in newer widgets (weather, Sun & Moon, Solar trend) plus footer elision hiding data age. Suggest a CI check that renders every widget at 800x480 and fails on clipped text.

## 2026-10-14 (UTC) QA run 3
- Last reviewed commit: dd9dab18555c64d82bd94594c35b72c8d8f86655 (PRs #116 to #168, 81 commits since a695f65).
- Build and 104/104 tests pass on main. Rendered 27 pane widgets at 800x480 and 1024x600 (light and dark) plus the default layout at 1920x1080; looked at the new EME, eclipse, Dst, NOAA scales, space-impact, radio, GPS, ADIF, OTA and fires panes. All fit their slots with no clipping or scrolling.
- Opened: none. Closed: #120 (X-ray axis letters fixed by #126, confirmed at 1920x1080). Still open: #150, #151, #152 (owner-filed).
- Diff scan: no QtWidgets in widgets, no http:// URLs, no passwords in QML or logs; UDP spot input binds to localhost unless LAN is chosen (off by default); FIRMS uses the public CSV, no key. Per-widget code was not read line by line.
- Minor, not filed: the card footer text ("Computed offline from the sys...", "...unreachable: No fixture for...") elides at 800x480; the age stays visible. The station column is half empty at 1920x1080.
- Health: layout is stable and the earlier text-fit issues are fixed; new panes behave honestly (Set up / Offline states when sources are missing). Remaining risk is data freshness on the real Pi (#150).

## 2026-10-14 (UTC) QA run 4
- Last reviewed commit: 7a4b360f9a322b1ab6a801df7a9ab357b57735ac (PRs #169 to #202, 71 commits since dd9dab1).
- Build and 124/124 tests pass on main. Rendered all 34 pane widgets at 800x480 and 1024x600 (light and dark) plus the default layout at 1920x1080. The truncation guard from #196 flags only quakes and fires.
- Opened: #203 [H] Quakes and Fires "Nearest ..." lines truncated at 800x480 and 1024x600. Closed: none. Still open: #150, #151, #152 (owner-filed).
- Diff scan: no QtWidgets, no http:// URLs, no passwords in QML or logs (telnet password stays in C++); the only ListView/Flickable uses are the time zone picker and settings. Two test assertions were changed but both follow intended behaviour changes (satellite default of two sub-points, stale Kp value). Per-widget code was not read line by line.
- Health: layout stays one-page with no scrolling and the new panes show honest Stale / Set up states; the only text-fit regressions are the two hazards panes. Fixture-only renders show DX weather as missing because the fixture set has one weather file, which is not a product bug.

## 2026-10-14 (UTC) QA run 5
- Last reviewed commit: 10253097793bfeb649bad0bd19110ac027a2c91d (PRs #204 to #227, 48 commits since 7a4b360).
- Build and 132/132 tests pass on main. Rendered all 35 pane widgets at 800x480 and 1024x600 (light and dark) and the default layout at 1920x1080, with no station configured so most panes show their Set up state. No text truncation was reported and nothing scrolls. Quakes and Fires render without truncation, so #203 is fixed on main. Still open: #150, #151, #152 (owner-filed).
- Opened: #229 [A] Main.qml safeArea bindings throw TypeErrors at shutdown (SafeArea is destroyed before the QML engine, from #216).
- Diff scan: test changes are additive only; no weakened assertions seen. Per-widget code was not read line by line. Panes were not rendered with a configured station, so the data-filled layouts of new panes (sun/moon extras, weather clouds, storm forecast) were not reviewed this run.
- Health: build, tests and layout are stable and new panes stay honest (Set up / Offline / Stale states). The only new defect is a harmless but noisy shutdown warning.

## 2026-10-14 (UTC) QA run 6
- Last reviewed commit: 36311105586b1b37cf7d982ed7fd2ec3f050b412 (PRs #228 to #259, 58 commits since 1025309).
- Build and 137/137 tests pass on main. Rendered the default layout at 800x480 and 1024x600 (light and dark) and 1920x1080 (dark), with a configured test station, plus every pane widget at 800x480 and 1024x600 (dark). The truncation guard reported nothing and nothing scrolls. Weather shows Loading because the fixture set has no matching weather file (fixture gap, not a product bug). Contact sheets were only skimmed this run, so the data-filled layouts of the new panes were not checked closely.
- Opened: #261 [A] callsign shown twice in the station card; #262 [G] map labels still collide with symbols, the path line and map buttons. Closed: none. Still open: #150, #151, #152, #246, #247 (owner-filed; #250 and #254 merged toward #246 and #247, not verified closed by me).
- Diff scan: no QtWidgets, no scrolling views, no passwords in QML or logs, no weakened assertions (no removed test assertions). The ADS-B model reads http://127.0.0.1:8080 (local PiAware only, off unless a receiver exists).
- Health: layout is stable and honest (Set up / Stale / Live states). Only cosmetic issues were found this run.

## 2026-10-14 (UTC) QA run 7
- Last reviewed commit: 13cfb911cd10219c41966928e749ddc77a0e71a7 (PRs #260 to #292, 65 commits since 3631110).
- Build and 142/142 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (light and dark, configured test station) and every pane widget at 800x480 and 1024x600 (dark); no truncation or TypeError was logged and nothing scrolls. Looked closely at Sun & Moon (moon disc), Next eclipse, GPS, Space weather impact and the map; all fit and show honest Live / Set up / Stale states. Eclipse date (6 Feb 2027 annular) and sunrise/sunset values are plausible.
- Opened: none. Closed: none (#261 and #262 are already closed). Still open: #150, #151, #152, #246, #247, #269, #281 (owner-filed or owner-input).
- Diff scan: no QtWidgets, http://, scrolling views or password handling in added code; no removed test assertions. Per-widget code was not read line by line; the footer text still elides at 800x480 (minor, not filed).
- Health: layout is stable and the new panes behave honestly. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-15 (UTC) QA run 8
- Last reviewed commit: eec6c48f59f65b185b7064bf5a5cf145ed214b5d (PRs #293 to #323, 53 commits since 13cfb91).
- Build and 145/145 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (light and dark, configured test station) and 37 pane widgets at 800x480 and 1024x600 (dark; contact sheets viewed at 1024x600). Nothing scrolls and no warnings were logged. bandactivity and watchlist have no pane id to render.
- Opened: #325 [D] daylight chart vertical jumps at UTC wrap; #326 [E] weather header labels overlap (Metric label has no elide). Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 (owner-filed or owner-input).
- Minor, not filed: station card "Short path · 5573 KM · 51 DEG / TRUE" wraps with an orphan "·"; Sun & Moon DX column touches the pane edge at 800x480; map labels still sit on markers.
- Diff scan: no QtWidgets, http://, scrolling views or password handling in added code; changed test assertions all follow intended behaviour additions. Per-widget code was not read line by line.
- Health: build and tests are green and panes stay honest (Live / Stale / Set up / Offline). Two cosmetic defects found; remaining risk is data freshness on the real Pi (#150).

## 2026-10-15 (UTC) QA run 9
- Last reviewed commit: 7ea1ba5e5fa810cc653af20e42f547df99969888 (PRs #324 to #346, 40 commits since eec6c48).
- Build and 148/148 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (light and dark, configured test station) and 39 pane widgets at 800x480 and 1024x600 (dark; Planets, Next eclipse, PSKReporter, Weather viewed closely). Nothing scrolls and no QML warnings were logged. The only truncation report is the Weather header "DE · San Francisco, US" in its Loading state, caused by the fixture set not matching the test station (fixture gap, not a product bug).
- Opened: none. Closed: none (#325 and #326 are already closed). Still open: #150, #151, #152, #246, #247, #269, #281 (owner-filed or owner-input).
- Diff scan: no QtWidgets, http://, scrolling views, new Qt-version-gated APIs or password handling in added code; the two removed test assertions (planet marker count, air line) follow the intended behaviour additions. Per-widget code was not read line by line; the new Timers page and alarms were not rendered.
- Health: build and tests are green, layouts fit and panes stay honest (Live / Stale / Set up / Offline). No new defects found; remaining risk is data freshness on the real Pi (#150).

## 2026-10-15 (UTC) QA run 10
- Last reviewed commit: bc889c676e8a7832623326d2615febce725c19ff (PRs #347 to #367, 41 commits since 7ea1ba5).
- Build and 151/151 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (light and dark, configured test station) and the new hams.at and GPS panes at 800x480 (dark); both fit and show honest Live / Set up states. The other panes were not re-rendered this run.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: no QtWidgets, scrolling views or password handling in added code; no removed test assertions. New network code: hams.at (HTTPS, fixture present) and the ADS-B endpoint now configurable via options.ini (host and port validated, falls back to 127.0.0.1:8080, http only to the user's own receiver). GPS serial device path is regex-restricted to /dev/tty* and /dev/serial/*. Per-widget code was not read line by line.
- Health: build and tests are green, layouts fit and panes stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-15 (UTC) QA run 11
- Last reviewed commit: ecf3fa7a273326dced802870d213b7c03e14a15b (PRs #368 to #382, 30 commits since bc889c6).
- Build and 152/152 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (light and dark, no station file took effect so the station card shows Set up); header, Space weather, Solar wind, DX cluster and map fit with honest Live / Stale / Set up states and nothing scrolls. The new Settings page (blanking, time zone search), Weather card changes and WSPR overlay were not rendered this run.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: no QtWidgets, http://, scrolling views or password handling in added code; no removed test assertions. Screen blanking is software-only (black overlay) with unit tests; any options change calls touch() and so wakes a blanked screen (minor, not filed). WSPR parser ships with a fixture. Per-widget code was not read line by line.
- Health: build and tests are green and layouts stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-15 (UTC) QA run 12
- Last reviewed commit: 286a07897de0a623ec659c1c9393524827e3d3e3 (PRs #383 to #398, 34 commits since ecf3fa7).
- Build and 154/154 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (light and dark, configured test station): nothing scrolls, panes show honest Live / Stale / Set up states, footers elide with an ellipsis at 800x480 (known, minor). The phone and landscape-phone layouts, WSPR-of-DE, satellite transmitter and footprint changes were not rendered this run.
- Opened: [A] station card path line wraps with an orphan "·" (first noted in run 8). Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: no QtWidgets, scrolling views, password handling, new http:// or removed test assertions in added code. The tvOS SatelliteStore.swift fetches CelesTrak over HTTPS with no fixture or offline test (Swift side, minor, not filed). Per-widget code was not read line by line.
- Health: build and tests are green and layouts stay honest. One cosmetic defect filed; remaining risk is data freshness on the real Pi (#150).

## 2026-10-15 (UTC) QA run 13
- Last reviewed commit: 7e43bd20acfc43f6b493f69094037cf234b0d415 (PRs #402 to #412, 22 commits since 286a078).
- Build and 157/157 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (light and dark): nothing scrolls, no QML warnings, panes show honest Live / Stale / Set up states and footers elide cleanly. The station file in the test config dir did not take effect (card shows Set up), so the #403 path-line wrap fix, Settings saved configurations, band-activity pane and alarms persistence were not rendered.
- Opened: none. Closed: none (the run 12 station-path wrap issue is no longer open after #403). Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: no QtWidgets, http://, scrolling views or password handling in added code; no weakened tests (the removed path_text assertions were rewritten with #403). Saved configurations (config_store.hpp) exclude station and credentials and validate names; UDP multicast group is validated IPv4 multicast only. Per-widget code was not read line by line.
- Health: build and tests are green and layouts stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-16 (UTC) QA run 14
- Last reviewed commit: f13cbb3 (PRs #413 to #422, 23 commits since 7e43bd2).
- Build and 157/157 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (light and dark, configured test station that took effect): nothing scrolls, panes show honest Live / Stale / Set up states. The Big Clock page, Settings, TLE editor and the new satellite/launch/EME changes were not rendered.
- Opened: [G] Venus and Mercury map labels overlap. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: no QtWidgets, http:// or password handling in added code; no removed test assertions. A ScrollView was added inside the satellites TLE text editor (field scroll, not page scroll; not filed). User TLE saving is size-limited, validated and written atomically beside options.ini.
- Health: build and tests are green and layouts stay honest. One cosmetic defect filed; remaining risk is data freshness on the real Pi (#150).

## 2026-10-16 (UTC) QA run 15
- Last reviewed commit: 848dcd6 (PRs #424 to #436, 24 commits since f13cbb3).
- Build and 157/157 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (light and dark, configured test station that took effect) plus the satellites, launches and EME panes: nothing scrolls, states are honest (Live / Stale / Set up / Offline), the Venus/Mercury labels no longer overlap at 1024x600. The EME Apsis row only appears when the pane is tall enough (by design). Launch-pad map labels can be long (e.g. "Martian Moons eXploration (MMX)"), minor, not filed.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: no QtWidgets, http://, scrolling views, password handling or removed test assertions in added code. Per-widget code was not read line by line.
- Health: build and tests are green and layouts stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-16 (UTC) QA run 16
- Last reviewed commit: 35594b2 (PRs #437 to #444, 15 commits since 848dcd6).
- Build and 156/156 tests pass on main (one fewer than run 15: qt-dashboard-regression went with the retired legacy dashboard in #444, expected). Rendered the default layout at 800x480 and 1024x600 (light and dark, configured test station that took effect): nothing scrolls, panes show honest Live / Stale / Set up states. The Sun & Moon next-season row, Settings Exit card and 1920x1080 were not re-inspected this run.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: no QtWidgets, http://, scrolling views or password handling in added code; no weakened tests. Season times (#442) are computed offline (Meeus) and checked against 2026 almanac values within 3 minutes. Settings Exit (#438) needs a second tap and is hidden on iOS. Minor, not filed: sun_regression.cpp prints its "checks, failures" line before the new season checks run.
- Health: build and tests are green and layouts stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-16 (UTC) QA run 17
- Last reviewed commit: 02cab7f (PRs #445 to #449, 10 commits since 35594b2).
- Build and 156/156 tests pass on main. Rendered the default layout at 800x480 dark (unconfigured station): nothing scrolls, the shortened station hints (#447) fit without clipping, panes show honest Live / Set up states. Other sizes and light theme not re-rendered (only two strings changed in src/).
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: the only src/ changes are two hint strings; the rest is tvOS Swift (map DX view, quake detail) and docs. No QtWidgets, http:// or weakened tests. Swift not reviewed line by line.
- Health: build and tests are green and layouts stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-16 (UTC) QA run 18
- Last reviewed commit: aad0012 (PRs #450 to #453, 7 commits since 02cab7f).
- Build and 156/156 tests pass on main. Rendered the default layout at 1024x600 and 800x480 (dark, unconfigured station) at text size Normal and 150 %: Normal is clean and nothing scrolls. At 150 % and 800x480 the Space weather title is elided and the footer reasons are cut. Light theme, 1920x1080 and 115/130 % not rendered.
- Opened: #454 [A] text size 150 % truncates pane chooser title at 800x480. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: text size (#452) is validated to 100/115/130/150 and tested for default, rejection and restart; map zoom test (#450) adds coverage, none removed. No QtWidgets, http:// or password handling. tvOS Swift not reviewed line by line.
- Health: build and tests are green and layouts stay honest at default settings. One minor opt-in layout defect filed; remaining risk is data freshness on the real Pi (#150).

## 2026-10-16 (UTC) QA run 19
- Last reviewed commit: 2c000a4 (PRs #455 to #459, 10 commits since aad0012).
- Build and 156/156 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (dark and light, unconfigured station): nothing scrolls, panes show honest Live / Stale / Set up states, the 800x480 titles are whole. #454 was already closed by #456 (pane badge drops to a dot when the title needs the width); 150 % text size was not re-rendered.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: no QtWidgets, http://, scrolling views or password handling in added code; no weakened tests. Moon bright-limb tilt (#457) uses the Meeus ch. 48 position angle minus the parallactic angle and has new regression checks; the visual orientation was not compared against a real sky chart. tvOS Swift not reviewed line by line.
- Health: build and tests are green and layouts stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).
