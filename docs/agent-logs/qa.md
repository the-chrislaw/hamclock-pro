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

## 2026-10-21 (UTC) QA run 47
- Last reviewed commit: 8259c98 (PRs #548 and #549 since a715353; #547 was my run 46 log). #548 adds the optional hams.at API key (C++-only `hamsatKey()`, Settings password field, bearer transport, 401 fallback to the public list, cache never shown as personalised, credentials file stays owner-only). #549 makes the Pi updater opt-in, stable-only and checksum-verified with rollback. Diffs reviewed: no QtWidgets, Qt 6.4 or QML credential exposure problems; new regression tests added, none weakened. Build and all 167 ctest tests pass on main. The hams.at pane and the new Settings card were not rendered (no screenshots this run), so their layout at 800x480 and 1024x600 is unchecked.
- Opened: none. Closed: none. Commented on #546: the release workflow does not publish a SHA256SUMS next to the .debs, so the new updater can never install anything. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues (#400 may now be answerable by #548).
- Health: main builds and passes tests; the credential handling in the new key feature looks right. Remaining risk is data freshness on the real Pi (#150) and the unchecked layout of the new Settings card.

## 2026-10-21 (UTC) QA run 48
- Last reviewed commit: e7128ed (PRs #551 and #552 since 8259c98; #550 was my run 47 log). #551 adds a rough landing ETA for descending sondes (`landing_eta_s`, Balloons detail line); #552 adds tomorrow's high/low to the Weather line (`forecast_days=2`, bounded parse, tests added). Diffs reviewed: fixture-served, no secrets, QtWidgets or Qt 6.4 problems, no tests weakened. Build and all 167 ctest tests pass on main. Default layout rendered at 800x480 and 1024x600, light and dark: no clipping. Balloons rendered with no data; Weather stayed on "Loading" (fixtures not matched in this render), so the longer "Tmrw" line is unchecked for fit at 800x480.
- Opened: #553 (#551 ETA ignores the report's age, so a 25-minute-old report still shows "landing in ~12 min"). Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes tests; layout is stable. Remaining risk is data freshness on the real Pi (#150) and unchecked fit of the longer Weather line.

## 2026-10-21 (UTC) QA run 49
- Last reviewed commit: 8734ffa (PRs #554 and #555 since e7128ed; both are agent log-only PRs, #554 my run 48 log). No source, test, workflow or widget change landed, so the build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #553, #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 48 (builds and passes tests then). Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather line.

## 2026-10-22 (UTC) QA run 50
- Last reviewed commit: a157fcd (PR #557 since 8734ffa; #556 was my run 49 log). #557 subtracts the report age from the Balloons landing ETA (new `landing_eta_remaining_s`, "probably landed" when elapsed). Diff reviewed: no network, secrets, QtWidgets or Qt 6.4 problems; regression checks added, none weakened. Build and all 167 ctest tests pass on main. No QML changed, so screenshots were not repeated; the "probably landed" wording was not rendered.
- Opened: none. Closed: #553 (verified in code and tests). Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes tests; behaviour is stable. Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather line.

## 2026-10-22 (UTC) QA run 51
- Last reviewed commit: 66d3825 (only #558, my run 50 log, since a157fcd). No source, test, workflow or widget change landed, so the build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 50 (builds and passes tests then). Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather line.

## 2026-10-22 (UTC) QA run 52
- Last reviewed commit: 76f310d (only #560 since 66d3825; #559 was my run 51 log). #560 is a CI-only change to `apple.yml`: the iPad Simulator step now reads the bundle id from the built app's Info.plist (the old hard-coded `org.hamclockpro.app` was wrong) and fails if the app is not running 15 s after launch. No source, tests, QML or widgets changed, so the build, tests and screenshots were not repeated. The step is still `continue-on-error` and its first real result was not seen, so #521 stays open (tvOS launch check is still missing).
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 50 (builds and passes tests then). Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather line.

## 2026-10-22 (UTC) QA run 53
- Last reviewed commit: 0409e87 (PR #562 since 76f310d; #561 was my run 52 log). #562 adds an approximate landing point for descending sondes (`landing_point()`, "lands ~N km DIR from you" in the Balloons detail line). Diff reviewed: pure C++ over the existing SondeHub feed, no new network code, secrets, QtWidgets or Qt 6.4 problems; regression checks added, none weakened. The previous-frame logic relies on SondeHub's frame keys iterating in time order (ISO datetimes, so true today). Build and all 167 ctest tests pass on main. There is no Balloons fixture in the repo, so the longer detail line could not be rendered and its fit at 800x480 is unchecked.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes tests; behaviour is stable. Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather and Balloons detail lines.

## 2026-10-22 (UTC) QA run 54
- Last reviewed commit: 6e76a77 (only #563, my run 53 log, since 0409e87). No source, test, workflow or widget change landed, so the build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 53 (builds and passes tests then). Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather and Balloons detail lines.

## 2026-10-22 (UTC) QA run 55
- Last reviewed commit: 0e801b3 (only #564, my run 54 log, since 6e76a77). No source, test, workflow or widget change landed, so the build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 53 (builds and passes tests then). Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather and Balloons detail lines.

## 2026-10-23 (UTC) QA run 56
- Last reviewed commit: 9e7e348 (only #565, my run 55 log, since 0e801b3). No source, test, workflow or widget change landed, so the build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 53 (builds and passes tests then). Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather and Balloons detail lines.

## 2026-10-23 (UTC) QA run 57
- Last reviewed commit: a8d5874 (only #566, my run 56 log, since 9e7e348). No source, test, workflow or widget change landed, so the build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 53 (builds and passes tests then). Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather and Balloons detail lines.

## 2026-10-23 (UTC) QA run 58
- Last reviewed commit: 39a44b5 (only #567, my run 57 log, since a8d5874). No source, test, workflow or widget change landed, so the build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 53 (builds and passes tests then). Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather and Balloons detail lines.

## 2026-10-23 (UTC) QA run 59
- Last reviewed commit: 1b07ae5 (only #568, my run 58 log, since 39a44b5). No source, test, workflow or widget change landed, so the build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 53 (builds and passes tests then). Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather and Balloons detail lines.

## 2026-10-23 (UTC) QA run 60
- Last reviewed commit: fecdf8e (#570 balloon flight trail, plus my run 59 log #569, since 1b07ae5). Build clean, 167/167 ctest pass. #570 diff reviewed: trail parse is bounds-safe, test added not weakened, WIDGETS.md updated, layer removed when stale; no contract violation found. Screenshots not repeated (map overlay only, no new layout).
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes all tests. Remaining risk is data freshness on the real Pi (#150) and the unchecked fit of the longer Weather and Balloons detail lines.

## 2026-10-23 (UTC) QA run 61
- Last reviewed commit: 175e353 (#575 astro Find-in-the-sky domain model, #576 Apple sky-pointer maths and iOS privacy strings, plus my run 60 log #571, since fecdf8e). Build clean, 169/169 ctest pass. Diffs reviewed: no UI or QML change (screenshots not repeated), tests added, WIDGETS.md updated and honest about what is not done, no network code, no secrets, angle wrapping and lock hysteresis look correct; no contract violation found.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes all tests. Remaining risk is data freshness on the real Pi (#150), the unchecked fit of the longer Weather and Balloons detail lines, and that the new sky-finder pane will need a 1024x600 layout check when it lands.

## 2026-10-24 (UTC) QA run 62
- Last reviewed commit: 611d775 (#577 Apple sky-finder attitude and sensor feed, #579 UI phone layout audit log, #580 astro pass-schedule include predicate, #581 Apple SkyFinder controller and haptic, plus my run 61 log #578, since 175e353). Build clean, 171/171 ctest pass. Screenshots not repeated (no QML or layout change). Tests were added, not weakened; no network code, no secrets.
- Opened: #582 ([F] sky sensor: extra CMMotionManager per available() call, dangling owner in handler, silent magnetic fallback, active flag set when sensor unavailable). Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes all tests. Remaining risk is iOS sensor code that Linux CI cannot exercise, data freshness on the real Pi (#150), and the 1024x600 fit of the sky-finder pane once it lands.

## 2026-10-24 (UTC) QA run 63
- Last reviewed commit: 96beb5c (#584 Passes pane schedule, #585 Apple sky sensor fixes, #586 Passes starred satellites and Starred filter, plus my run 62 log #583, since 611d775). Build clean, 173/173 ctest pass. Rendered 800x480, 1024x600 and 1920x1080 in both themes, plus the new Passes pane with live rows at 1024x600 (fits, no scroll, columns readable). #582 was already closed by #585 (verified in the diff).
- Opened: #587 ([A] 800x480 render shows the dashboard cropped, unstable between runs, not Passes-specific). Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds and passes all tests; 1024x600 is clean. Remaining risk is the 800x480 fit (#587), data freshness on the real Pi (#150), and the Passes pane's tap targets (small text-only filter and star hit areas) on the real touchscreen.

## 2026-10-24 (UTC) QA run 64
- Last reviewed commit: 9b313fa (#589 800x480 screenshot fix and telnet offline guard, #590 astro ISS-class magnitude, #591 Apple pass reminders, plus my run 63 log #588, since 96beb5c). Build clean, 176/176 ctest pass. Rendered 800x480, 1024x600 and 1920x1080 in both themes: 800x480 now shows the full dashboard (the #587 crop is fixed and the issue is closed), the other sizes are clean.
- Diffs reviewed: tests added, not weakened; telnet panes no longer open sockets under --offline/--fixtures; reminder planning clamps lead time and rejects past passes; no secrets or personal data.
- Opened: none. Closed: none (#587 was already closed by #589). Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds, passes all tests and renders correctly at all three sizes. Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and the Passes pane tap targets on the real touchscreen.

## 2026-10-24 (UTC) QA run 65
- Last reviewed commit: 00c7660 (#593 astro Passes phone-size render note, plus my run 64 log #592, since 9b313fa). Docs and agent-log changes only (WIDGETS.md one line, astro.md); no source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 64 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and the Passes pane tap targets on the real touchscreen.

## 2026-10-24 (UTC) QA run 66
- Last reviewed commit: 911ecd9 (#595 eclipse totality/annular contact times, #596 air temperature 3 h range caption, plus my run 65 log #594, since 00c7660). Build clean, 176/176 ctest pass. Rendered 800x480, 1024x600 and 1920x1080 in both themes: clean, no clipping (new Eclipse row only shows at rowsFit >= 6, range caption only when not compact).
- Diffs reviewed: tests added, no network code, no secrets; one test assertion is vacuous and one header comment is misplaced.
- Opened: #597 ([D] vacuous Dallas totality assertion, misplaced comment in sensors_read.hpp). Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds, passes all tests and renders correctly at all three sizes. Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-24 (UTC) QA run 67
- Last reviewed commit: f7aafc3 (#599 eclipse test fix for #597, plus my run 66 log #598, since 911ecd9). Build clean, 176/176 ctest pass. Test-only change; no QML or layout change, so screenshots were not repeated. The Dallas assertion now checks the real outcome and an annular case was added.
- Opened: none. Closed: none (#597 stays open: the misplaced `format_temp_range` comment in sensors_read.hpp is not yet fixed; commented). Still open: #597, #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds, passes all tests and is unchanged in behaviour since run 66. Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-25 (UTC) QA run 68
- Last reviewed commit: 0260a9b (#601 Big Clock analog face option, plus my run 67 log #600, since f7aafc3). Build clean, 176/176 ctest pass. Rendered the Big Clock page with the analog face at 800x480, 1024x600 and 1920x1080 in both themes (local-only QML tweak to open the page, reverted): dial, hands and digital lines fit with no clipping or scroll.
- Diffs reviewed: setting validated and persisted with a restart test, no network code, no secrets; dial is UTC-only and repaints on time ticks (not on theme change, minor).
- Opened: none. Closed: none. Still open: #597, #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main builds, passes all tests and renders correctly at all three sizes. Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-25 (UTC) QA run 69
- Last reviewed commit: d36f59a (only my run 68 log #602 merged since 0260a9b). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #597, #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-25 (UTC) QA run 70
- Last reviewed commit: 0b57808 (#606 hide TV overscan and screen blanking cards on iOS, plus agent logs #603-#605, since d36f59a). The only source change is two `visible: Qt.platform.os !== "ios"` lines in SettingsPage.qml; behaviour on Linux/Pi is unchanged, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #597, #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-25 (UTC) QA run 71
- Last reviewed commit: 7ba1b92 (#608 move `format_temp_range` above the PeakHistory comment, plus my run 70 log #607 and wx log, since 0b57808). The only source change is a comment move in sensors_read.hpp; no behaviour change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: #597 (verified on main: #599 fixed the Dallas assertion and added an annular case, #608 fixed the comment). Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-25 (UTC) QA run 72
- Last reviewed commit: dca3264 (only my run 71 log #609 merged since 7ba1b92). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-25 (UTC) QA run 73
- Last reviewed commit: 45883f7 (only my run 72 log #610 merged since dca3264). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-26 (UTC) QA run 74
- Last reviewed commit: f5ec7e9 (only my run 73 log #611 merged since 45883f7). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-26 (UTC) QA run 75
- Last reviewed commit: 17b3474 (only my run 74 log #612 merged since f5ec7e9). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-26 (UTC) QA run 76
- Last reviewed commit: 4b8c6a0 (only my run 75 log #613 merged since 17b3474). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-26 (UTC) QA run 77
- Last reviewed commit: 3478047 (only my run 76 log #614 merged since 4b8c6a0). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-26 (UTC) QA run 78
- Last reviewed commit: d25df15 (only my run 77 log #615 merged since 3478047). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-26 (UTC) QA run 79
- Last reviewed commit: c533308 (only my run 78 log #616 and the UI agent's run 72 log #617 merged since d25df15). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-27 (UTC) QA run 80
- Last reviewed commit: f123673 (only my run 79 log #618 merged since c533308). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-27 (UTC) QA run 81
- Last reviewed commit: 4d8c122 (only my run 80 log #619 merged since f123673). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-27 (UTC) QA run 82
- Last reviewed commit: 7045234 (only my run 81 log #620 merged since 4d8c122). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-27 (UTC) QA run 83
- Last reviewed commit: a27999d (only my run 82 log #621 merged since 7045234). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-27 (UTC) QA run 84
- Last reviewed commit: 1a93bb8 (only my run 83 log #622 merged since a27999d). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-27 (UTC) QA run 85
- Last reviewed commit: 2f9220c (merged since 1a93bb8: my run 84 log #623 and docs-only #624, App Store privacy/support pages). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-28 (UTC) QA run 86
- Last reviewed commit: 87391b1 (only my run 85 log #625 merged since 2f9220c). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68 (builds, passes 176/176 tests, renders correctly at all three sizes). Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-28 (UTC) QA run 87
- Last reviewed commit: 10aea08 (merged since 87391b1: my run 86 log #626 and #627, pure-maths camera AR projection for Find-in-the-sky, #573). Reviewed #627: projection, roll direction, zenith and behind-camera handling are correct, new test is not weakened, no QtWidgets/new Qt APIs or network code. Main builds and passes 177/177 tests. No QML or Linux-visible UI change, so screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68. Remaining risk is iOS code Linux CI cannot exercise (the camera layer that will consume #627), data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-28 (UTC) QA run 88
- Last reviewed commit: 00eb867 (only my run 87 log #630 merged since 10aea08). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68. Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-28 (UTC) QA run 89
- Last reviewed commit: 9f704d0 (only my run 88 log #631 merged since 00eb867). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68. Remaining risk is iOS code Linux CI cannot exercise (the camera layer that will consume #627), data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-28 (UTC) QA run 90
- Last reviewed commit: 43db071 (merged since 9f704d0: my run 89 log #632 and #633, aspect-fill camera preview FOV maths, #573). Reviewed #633: focal length, portrait rotation and aspect-fill crop are correct, invalid input falls back to defaults, new test is not weakened, no QtWidgets/new Qt APIs/network code. Main builds and passes 178/178 tests. No QML or Linux-visible UI change, so screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi since run 68. Remaining risk is iOS code Linux CI cannot exercise (the camera layer that will consume #627/#633), data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-28 (UTC) QA run 91
- Last reviewed commit: 3f7f9cc (merged since 43db071: #628 adaptive pane grid, #634 sky finder left out of iOS builds, #635 privacy policy, my run 90 log). Main builds and passes 182/182 tests; #628 extends the pane tests rather than weakening them, no QtWidgets/new Qt APIs/network code. Screenshots at 800x480, 1024x600 and 1280x720 are correct.
- Opened: #638 ([A] pane grid renders empty at 1920x1080). Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: the Pi sizes are fine, but #628 left the pane area blank at 1920x1080 in both themes, the first visible regression since run 68. Remaining risk is iOS code Linux CI cannot exercise, data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-29 (UTC) QA run 92
- Last reviewed commit: 8507bee (merged since 3f7f9cc: #637 AVFoundation camera FOV reader, #640 phone Settings fit, #641 phone layout with two panes and compact station card, #644 version 1.0.0, #642 tvOS TestFlight upload, #646 pane grid 1920x1080 fix, my run 91 log). Main builds and passes 184/184 tests; diffs add no QtWidgets, scrolling containers, password exposure or network code, and the one test change is an added assertion. Screenshots at 800x480, 1024x600 and 1920x1080 in light and dark are correct, with no clipping beyond known truncation of stale footers at 800x480.
- Opened: none. Closed: none (#638 was already closed by #646; verified fixed on main, 1920x1080 now shows five panes plus the map). Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is healthy and the 1080p regression is resolved. Remaining risk is iOS/tvOS code Linux CI cannot exercise (phone layout, camera layer), data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-29 (UTC) QA run 93
- Last reviewed commit: 1df00d0 (merged since 8507bee: #645 iPhone/iPad parity pass, my run 92 log #647). Main builds and passes 187/187 tests. Reviewed #645: Keychain secret store is only set on Apple builds (Pi still uses owner-only credentials.ini), writeAccounts leaves the file alone if the store refuses, passwords are not exposed to QML or logs, per-config-dir keys keep scratch profiles from touching real ones; tall-area pane grid only affects height > width so Pi sizes are unchanged, pane tests gained assertions, no QtWidgets/new Qt APIs/network code. Screenshots at 800x480 and 1024x600 in light and dark are unchanged and correct, with only the known truncation of stale footers at 800x480.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is healthy and unchanged in behaviour on the Pi. Remaining risk is iOS/tvOS code Linux CI cannot exercise (Keychain store, phone layout, camera layer), data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-29 (UTC) QA run 94
- Last reviewed commit: 0572e03 (only my run 93 log #649 merged since 1df00d0). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi. Remaining risk is iOS/tvOS code Linux CI cannot exercise (Keychain store, phone layout, camera layer), data freshness on the real Pi (#150), and Passes pane tap targets on the real touchscreen.

## 2026-10-29 (UTC) QA run 95
- Last reviewed commit: 01ce85e (merged since 0572e03: #648 adaptive panes for every widget plus audit fixes, #651 tvOS version 1.0.0, my run 94 log #650). Main builds; 186 of 187 tests pass. `qml-app-truncation-1512x982` fails here (exit 4, Sun & Moon status badge clipped at the card edge in the spill pane). Reviewed #648: no QtWidgets, scrolling containers, password exposure or network code seen in the diff; pane tests gained assertions. Screenshots at 800x480, 1024x600 and 1920x1080 in light and dark are correct.
- Opened: #652 ([A] Sun & Moon badge clipped at 1512x982). Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: the Pi sizes are fine. #648 left a desktop-size clipping regression in the Sun & Moon spill pane, which CI did not catch, so the guard may depend on fonts. Remaining risk is iOS/tvOS code Linux CI cannot exercise and data freshness on the real Pi (#150).

## 2026-10-29 (UTC) QA run 96
- Last reviewed commit: 82b125d (merged since 01ce85e: #656 status badge width tracks dotOnly, my run 95 log #653). Main builds and passes 187/187 tests, including `qml-app-truncation-1512x982`, so #652 is fixed on main and already closed. #656 is a one-line change in StatusBadge.qml with no QtWidgets, scrolling, network or credential code. Screenshots were not repeated for this one-line change.
- Opened: none. Closed: none (#652 was already closed). Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is healthy and the 1512x982 clipping regression is resolved. Remaining risk is iOS/tvOS code Linux CI cannot exercise and data freshness on the real Pi (#150).

## 2026-10-29 (UTC) QA run 97
- Last reviewed commit: fa180ce (only my run 96 log #658 merged since 82b125d). No source, test, QML or workflow change, so build, tests and screenshots were not repeated.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is unchanged in behaviour on the Pi. Remaining risk is iOS/tvOS code Linux CI cannot exercise and data freshness on the real Pi (#150).

## 2026-10-30 (UTC) QA run 98
- Last reviewed commit: d997638 (merged since fa180ce: #655 iPhone widgets, #659 tvOS parity rebuild, #661 iPad portrait map, #668 Apple attribution, #667 weather place wrapping, #666 map menu position, #664 Mac shortcuts, #663 Mac sleep/wake, #662 tvOS off-main parse, plus CI/TestFlight fixes, my run 97 log). Main builds and passes 193/193 tests. Qt-side diffs (28 files under src) add no QtWidgets, scrolling containers, password exposure or network code, and no test assertions were removed. Screenshots at 800x480, 1024x600 and 1920x1080 in light and dark are correct; stale/set-up states are honest and nothing is clipped beyond the known truncated stale footers at 800x480. Most of the diff is Swift (Apple apps), which Linux CI cannot exercise.
- Opened: none. Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is healthy and unchanged in behaviour on the Pi. Remaining risk is iOS/tvOS/Mac code Linux CI cannot exercise and data freshness on the real Pi (#150).

## 2026-10-30 (UTC) QA run 99
- Last reviewed commit: 7a40add (merged since d997638: #671 satellite pass alerts, #636 bands phone fit, #675/#676/#677/#680/#684/#689, #691/#692/#693 unit and clock fixes, #700 device location, my run 98 log). Main builds; 203 of 204 tests pass. `widget-satellites-sat_alert_service_regression` times out at 60 s under ctest; run directly it passes (32 checks) but takes about 97 s. Screenshots at 800x480 and 1024x600 (light, dark) are correct, with only the known truncated stale footers at 800x480. The 1920x1080 shots rendered without error but were not inspected.
- Opened: #715 ([D] sat alert service test takes ~97 s, times out). Closed: none. Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is healthy on the Pi sizes, but #671 added a test slow enough to make CI flaky and a possible slow pass recompute on the Pi. Remaining risk is Apple code Linux CI cannot exercise and data freshness on the real Pi (#150).

## 2026-10-30 (UTC) QA run 100
- Last reviewed commit: e844520 (merged since 7a40add: #720 sat alert test 97 s to 7 s, #704 station file load error, #705 time zone follows station location, #706 poll rates, #673 data polling robustness, #717 macOS location, #703 Mac menu bar, #688 phone swipe carousel, tvOS/iPhone QA fixes, #681 release pipeline). Main builds and passes 214/214 tests, including the sat alert test (#715 is fixed by #720). The src diff adds no QtWidgets, scrolling containers or password exposure, and no test assertions were removed. Screenshots at 800x480 and 1024x600 (light, dark) are correct; stale and set-up states are honest.
- Opened: none. Closed: none (#715 was already closed). Still open: #546, #531, #532, #527, #526, #521, #513 plus the owner-input issues.
- Health: main is healthy on the Pi sizes and CI is no longer flaky on the sat alert test. Remaining risk is Apple code Linux CI cannot exercise and data freshness on the real Pi (#150).
