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
