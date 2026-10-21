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

## 2026-10-17 (UTC) QA run 20
- Last reviewed commit: 6895136 (PRs #460 to #463, 7 commits since 2c000a4).
- Build and 156/156 tests pass on main. The only non-tvOS change is a test (pane chooser must categorise every registered widget, #461); no Qt source changed, so no screenshots were re-rendered this run.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: tvOS Open-Meteo weather (#463) and HF band card (#462) are Qt-free C++ cores with new regression tests; no http://, secrets or personal data in added code. Swift not reviewed line by line.
- Health: build and tests are green. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-17 (UTC) QA run 21
- Last reviewed commit: ef6807c (PRs #464 to #472, 18 commits since 6895136).
- Build and 158/158 tests pass on main. Rendered the default layout at 800x480, 1024x600 and 1920x1080 (dark and light, unconfigured station): nothing scrolls, titles and footers fit, panes show honest Live / Set up states. New opt-in features (RBN spots, UDP sets DX, accent colour, satellite frequency column, EME QR) were not rendered. Startup prints one `qt.core.qobject.connect: invalid nullptr parameter` warning in offscreen mode; not traced, not filed.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: RBN (#468) is opt-in, receive-only, needs a callsign, uses a backoff reconnect and is bounded to 200 reports / 15 min, with regression tests. Vendored Nayuki QR encoder (#470) has a provenance doc and tests. No QtWidgets, http://, scrolling views or password handling in added code; no weakened tests. tvOS Swift not reviewed line by line.
- Health: build and tests are green and layouts stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-17 (UTC) QA run 22
- Last reviewed commit: 88af226 (PRs #473 to #481, 16 commits since ef6807c).
- Build and 160/160 tests pass on main. Rendered the default layout at 800x480 and 1024x600 (dark and light, unconfigured station, empty fixtures): nothing scrolls, titles and footers fit, panes show honest Offline / Set up states. 1920x1080 and the new Meshtastic pane with live data were not rendered.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: the new Meshtastic widget (#476/#478/#480) is opt-in, reads from a user-configured node, reports "live" only after a well-formed frame, drops its map layer when the link is lost, retries with backoff, and has regression tests. WWFF feed (#481) adds a fixture and tests. No QtWidgets, http://, scrolling views or password handling in added code; no weakened tests. tvOS Swift not reviewed line by line.
- Health: build and tests are green and layouts stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-17 (UTC) QA run 23
- Last reviewed commit: bc24808 (PRs #482 to #487, 11 commits since 88af226).
- Build and 160/160 tests pass on main. Rendered the default layout at 800x480 and 1024x600 (dark and light, unconfigured station, empty fixtures): nothing scrolls, titles and footers fit, panes show honest Offline / Set up states. The new IOTA group name on DX cluster rows (#485) was not rendered with live spots; it is hidden in compact mode.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: IOTA group names (#485) parse a downloaded JSON defensively (ref regex, name capped at 60 chars), poll weekly through the shared transport and have regression tests with a fixture. tvOS keep-awake, pixel shift and dim (#484, #487) are opt-in; Swift not reviewed line by line. No QtWidgets, http://, scrolling views or password handling in added code; no weakened tests.
- Health: build and tests are green and layouts stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-17 (UTC) QA run 24
- Last reviewed commit: f936b83 (PRs #488 to #490, 4 commits since bc24808).
- Build and 160/160 tests pass on main. Rendered the default layout at 800x480 and 1024x600 (dark and light, unconfigured station, empty fixtures): nothing scrolls, titles and footers fit, panes show honest Offline / Set up states. The new Maidenhead square lines (#490, zoom >= 3) were not rendered zoomed in.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Diff scan: #490 adds pure square-line geometry with count and field-exclusion checks and a zoom-gated render test; no QtWidgets, http://, scrolling views or password handling; no weakened tests. tvOS (#489) is a docs-only change.
- Health: build and tests are green and layouts stay honest. No new defects; remaining risk is data freshness on the real Pi (#150).

## 2026-10-17 (UTC) QA run 25
- Last reviewed commit: 878763c (PR #492 only, a docs-only agent log, plus my own run 24 log since f936b83).
- No source, test or workflow changes since run 24, so the build, tests and screenshots from run 24 still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new to review; main is unchanged in behaviour since run 24. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-18 (UTC) QA run 26
- Last reviewed commit: 79ca4d4 (PRs #493 and #494 only, both docs-only agent logs, since 878763c).
- No source, test or workflow changes since run 24, so the build, tests and screenshots from run 24 still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new to review; main is unchanged in behaviour since run 24. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-18 (UTC) QA run 27
- Last reviewed commit: e71f5ee (PRs #495 and #497 only, both docs-only agent logs, since 79ca4d4).
- No source, test or workflow changes since run 24, so the build, tests and screenshots from run 24 still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new to review; main is unchanged in behaviour since run 24. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-18 (UTC) QA run 28
- Last reviewed commit: a0cb397 (PR #498 only, my own run 27 log, since e71f5ee).
- No source, test or workflow changes since run 24, so the build, tests and screenshots from run 24 still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new to review; main is unchanged in behaviour since run 24. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-18 (UTC) QA run 29
- Last reviewed commit: dc8ab56 (PR #499 only, my own run 28 log, since a0cb397).
- No source, test or workflow changes since run 24, so the build, tests and screenshots from run 24 still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new to review; main is unchanged in behaviour since run 24. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-18 (UTC) QA run 30
- Last reviewed commit: d7b5fae (PRs #500 to #502 since dc8ab56). #501 is the only source change: Big Clock shows local sidereal time beside the auxiliary line (`StationModel::lstTime`, `BigClockPage.qml`). Diff reviewed: reuses `aux_time_text`, no duplicated maths, single elided caption line (no overflow or scroll), no network or credential code, Qt 6.4-safe. Build and all 160 ctest tests pass on main. No CLI path renders the Big Clock overlay in a screenshot, so its layout was checked from the QML only.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: main builds and passes tests; the new Big Clock LST line looks sound. Minor gap: no test covers `lstTime` or the dedup against the aux line. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-18 (UTC) QA run 31
- Last reviewed commit: f89b3ea (PR #503 only, my own run 30 log, since d7b5fae).
- No source, test or workflow changes since run 30, so the build, tests and screenshots from that run still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new to review; main is unchanged in behaviour since run 30. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-19 (UTC) QA run 32
- Last reviewed commit: da74df4 (PRs #504 and #505 since f89b3ea). #505 is the only source change: vendored truncated ELP2000-82B lunar theory (`src/domain/astro_elp.{hpp,cpp}`, `elp82b_terms.inc`, `elp_regression` test, third-party note with source and licence). Diff reviewed: domain-only, no UI, network, credential or QtWidgets code, Qt-independent, tests compare against full-theory values and Meeus 47.a with tight tolerances (not weakened), provenance documented. Cosmetic only: `elp_moon` computes the same `fmod` twice in the return statement. Not wired into the eclipse model yet, so no screenshot change. Build and all 161 ctest tests pass on main; screenshots from run 24 still apply (no QML changes).
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: main builds and passes tests; the ELP groundwork looks sound and is not yet user-visible. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-19 (UTC) QA run 33
- Last reviewed commit: bf08310 (PRs #506 and #507 since da74df4). #507 is the only source change: `eclipse_contacts()` solver (`src/domain/astro_eclipse_contacts.cpp`) on the vendored ELP moon, with `eclipse_contacts_regression`. Diff reviewed: domain-only, no UI, network, credential or QtWidgets code, Qt-independent, not yet wired into the Eclipse card (the WIDGETS.md note says so honestly). Tests compare with NASA greatest-eclipse times and totality durations (8 s tolerance for 2024 and 2027). The 2017 case uses a 100 s tolerance on a 160 s duration, which is loose but justified in a comment (Meeus Sun path offset); worth tightening once a better Sun is used. Build and all 162 ctest tests pass on main; no QML changes, so screenshots from run 24 still apply.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: main builds and passes tests; eclipse contact groundwork is sound and not yet user-visible. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-19 (UTC) QA run 34
- Last reviewed commit: f0a4d4b (PR #508 only, my own run 33 log, since bf08310).
- No source, test or workflow changes since run 33, so the build, tests and screenshots from earlier runs still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new to review; main is unchanged in behaviour since run 33. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-19 (UTC) QA run 35
- Last reviewed commit: 2313a2a (PRs #509 to #511 since f0a4d4b). Source changes: #510 truncated VSOP87D Sun (`astro_vsop.cpp`, generated `.inc`, generator script, `vsop_regression` vs Meeus 25.b, third-party note) now used by the eclipse contact solver, and #511 `AlarmsModel::reload()` wired to `optionsChanged` so restored configurations apply alarms at once (test added, Settings caption updated). Diffs reviewed: domain/UI-model only, no network, credential or QtWidgets code, tests not weakened, Qt 6.4-safe. The only QML change is one caption string, so earlier screenshots still apply. Build and all 163 ctest tests pass on main.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: main builds and passes tests; both changes look sound. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-19 (UTC) QA run 36
- Last reviewed commit: da566cd (PRs #512 to #515 since 2313a2a). Only non-doc change is #514: `apple.yml` now passes `-qmldir=src` to `macdeployqt` and smoke-tests the packaged app (checks `Resources/qml/QtQuick`, renders a 1024x600 `--offline` screenshot) in both the arm64 and universal jobs. Diff reviewed: workflow only, no secrets, sound. #515 is docs (eclipse residual notes, honest "not wired" wording kept). No source or QML changes, so the build, tests and screenshots from earlier runs still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new in behaviour; the Mac bundle fix closes a real packaging gap and now has a smoke test. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-19 (UTC) QA run 37
- Last reviewed commit: 4e4c4e4 (PRs #516 and #517 since da566cd). #517 is the only source change: `TextEntryHint` (`src/platform/apple_textentry.*`, iOS-only `.mm` using GameController) shows a warning under `FormField` text fields only when the iPad app runs on a Mac (#513); the message is empty everywhere else, so layouts are unchanged. Diff reviewed: no network, credential or QtWidgets code, tests added (`text-entry-hint-regression`), Qt 6.4-safe. Build and all 164 ctest tests pass on main; a 1024x600 dark `--offline` screenshot is unchanged and clean.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: main builds and passes tests; the new hint is inert off iOS. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-20 (UTC) QA run 38
- Last reviewed commit: a95a1d8 (PRs #518 and #519 since 4e4c4e4). #519 is the only source change: session-only demo mode (`AppSettings::demoMode`, Settings card, "Stop demo" in the pane long-press menu; `PaneSlot.qml` forces a 10 s rotation over all widgets while on). Diff reviewed: not persisted, saved rotations untouched (test added), no network, credential or QtWidgets code, Qt 6.4-safe. Cosmetic only: QML hard-codes `10` instead of using `AppSettings::kDemoSeconds`. Build and all 164 ctest tests pass on main; screenshots were not repeated (one new Settings card, no layout change to panes).
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: main builds and passes tests; demo mode looks sound and is off by default. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-20 (UTC) QA run 39
- Last reviewed commit: 32233eb (PR #520 only, my own run 38 log, since a95a1d8).
- No source, test or workflow changes since run 38, so the build, tests and screenshots from earlier runs still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new to review; main is unchanged in behaviour since run 38. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-20 (UTC) QA run 40
- Last reviewed commit: 0b74176 (PR #523 and #522 since 32233eb). #523 adds `packaging/apple/check_ios_plugins.sh` and an `apple.yml` step that fails the iOS build when a statically linked Qt Quick plugin is missing from the binary (#521). Diff reviewed: script and workflow only, no secrets, sound. #522 is my own run 39 log.
- No source or QML changes, so the build, tests and screenshots from earlier runs still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new in behaviour; the iOS plugin check closes a silent blank-app failure mode. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-20 (UTC) QA run 41
- Last reviewed commit: 511accd (PR #524 only, my own run 40 log, since 0b74176).
- No source, test or workflow changes since run 40, so the build, tests and screenshots from earlier runs still apply; they were not repeated.
- Opened: none. Closed: none. Still open: #150, #151, #152, #246, #247, #269, #281 plus the owner-input issues.
- Health: nothing new to review; main is unchanged in behaviour since run 38. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-20 (UTC) QA run 42
- Last reviewed commit: d028a93 (PRs #528, #533, #534 since 511accd). #533 (sun/moon) and #534 (sensors, weather) are QML-only fit fixes for #529/#530: shorter DE/DX headers and values, `HorizontalFit` plus elide, a smaller Moon disc, a "Clock" caption that elides, and "DE"/"DX" weather labels. #528 is the 1.0 release-focus doc. Diffs reviewed: no network, credential, QtWidgets or Qt 6.4 problems; no tests changed. Build and all 164 ctest tests pass on main. Rendered Sun & Moon, Weather and Host sensors with a configured station (offline fixtures) at 800x480 and 1024x600, light and dark: no clipping; Moon rise/set and season rows drop out at 1024x600 by design. Sensors Clock row and weather data were not exercised (no fixture for them in this render); 1920x1080 not repeated.
- Opened: none. Closed: #529 (fixed by #533, verified by screenshots at 800x480 and 1024x600). Still open: #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes tests; the small-screen clipping fixes look good. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-20 (UTC) QA run 43
- Last reviewed commit: bc3a43d (PRs #536 and #537 since d028a93; #535 was my run 42 log). #536 extends the truncation guard in `src/ui/main.cpp` to report text clipped by its card or window (exempt: `clipOk`, `elideOk`, flickables) and adds 1024x600 to the `qml-app-truncation-*` tests. #537 wraps Space alerts headlines (two lines max) and makes the Sun image decode synchronous. Diffs reviewed: QML and test-harness only, no network, credential, QtWidgets or Qt 6.4 problems, no tests weakened. Build and all 165 ctest tests pass on main. Default layout rendered at 800x480 and 1024x600 (dark): no clipping; the Space alerts and Sun cards were not rendered with data (no fixture wired into that render).
- Opened: none. Closed: none. #531 and #532 stay open: the guard still only walks the default layout, not every pane id as #532 asks, and #531's "age reflects the image's real fetch time" part is not addressed by #537.
- Still open: #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes tests; the stricter guard is a good safety net. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-21 (UTC) QA run 44
- Last reviewed commit: 02c2a3d (PRs #539 and #540 since bc3a43d; #538 was my run 43 log). #539 adds the Balloons pane (SondeHub telemetry, `src/widgets/balloons/`, map layer, fixture and tests); #540 is the README and screenshots. Diffs reviewed: fixture-served, no secrets or QtWidgets, Qt 6.4-safe, no tests weakened. Build and all 167 ctest tests pass on main. Rendered the Balloons pane at 800x480 and 1024x600 (dark, configured station, stale fixture): layout fits, but the empty-state message is never shown (#541). Not checked: the size of the real one-hour global SondeHub payload on a Pi, and Balloons with fresh data (fixture is hours old).
- Opened: #541 (Metric hides the caption when unavailable). Closed: none. Still open: #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes tests. The new Balloons pane is sound apart from the invisible empty-state text. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-21 (UTC) QA run 45
- Last reviewed commit: 6492ec9 (PR #543 since 02c2a3d; #542 was my run 44 log). #543 moves the Balloons empty-state text out of the Metric caption into its own `Text` in `BalloonsCard.qml` (fixes #541). Diff reviewed: QML only, no network, credential, QtWidgets or Qt 6.4 problems, no tests changed. Build and all 167 ctest tests pass on main. Rendered Balloons at 1024x600 (dark, configured station, fixtures): "None heard in the last 30 min" is now visible and nothing clips. 800x480 rendered but not inspected closely; light theme and 1920x1080 not repeated.
- Opened: none. Closed: #541 (verified by screenshot). Still open: #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes tests; the Balloons empty state now works. Remaining risk is data freshness on the real Pi (#150).

## 2026-10-21 (UTC) QA run 46
- Last reviewed commit: a715353 (PR #545 since 6492ec9; #544 was my run 45 log). #545 adds the pi-gen image scaffold, kiosk and updater units, `image.yml` and a `--first-run` flag that opens Settings when no station is configured. Diff reviewed: the C++/QML change is tiny and uses existing properties (`station.configured`, `settingsLayer`); no QtWidgets, Qt 6.4 or credential problems; no tests changed. The build, tests and screenshots were not repeated (no widget change). The image itself was not built or run.
- Opened: #546 (the root hourly updater installs an unverified .deb, including the rolling pre-release). Closed: none. Still open: #531, #532, #527, #526 (image unvalidated on a device), #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour for widgets; the new packaging is a security concern until the update path is verified. Remaining risk is data freshness on the real Pi (#150).
