# Agent log: space

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (UTC): compact layouts (issue #78)

- Space weather and Solar wind cards now have a `Card.compact` layout: one line per value (Kp, SFI, X-ray) and one row per wind/Bz with a sparkline, so nothing clips at 800x480 or 1024x600. Full layouts unchanged in larger panes.
- Note: Layout-based rows with fill-width Text inside the card body caused a layout recursion crash at 800x480; compact rows use plain Column/Item.
- Verified by offscreen screenshots at 800x480, 1024x600, 1920x1080; not validated on device.
- Next: backlog item 5 (SDO imagery), then 6 (NOAA alerts).

## 2026-10-13 (UTC): SDO solar imagery (backlog 5)

- New `solarimage` widget ("Sun (SDO)"): latest NASA SDO AIA 193, AIA 304 and HMI intensitygram. Tap the image to cycle; only the shown channel is polled (every 15 min); last image is cached on disk and shown as stale when old. Non-image bodies (e.g. an HTML error page) are rejected with an error state. Source terms recorded in the roadmap's Data source policy.
- Image is passed to QML as a `data:` URL, so no shared image-provider or main.cpp change was needed.
- Verified by offline tests and offscreen screenshots at 800x480 and 1920x1080 (compact card fits); not validated on device. The sample fixture is a one-time capture; reachability of sdo.gsfc.nasa.gov from the sandbox was not checked.
- Next: backlog 6 (NOAA alerts/watches feed), then 11 (X-ray 24 h plot).
- Note: PR #73 (docs, not mine) currently has merge conflicts and was left alone.

## 2026-10-13 (UTC): NOAA space alerts (backlog 6)

- New `solaralerts` widget ("Space alerts"): SWPC `products/alerts.json` polled every 10 min. Shows only what is in force: newest message per product code, cancellations drop their product, warnings until their end time, alerts/summaries for 24 h, watches for 48 h. Empty list from fresh data reads "Nothing in force"; no data reads loading/offline. Expiry follows the clock between fetches.
- Compact card shows two rows, full card five with kind/issued/until. Checked by offscreen screenshots at 800x480, 1024x600, 1920x1080 (a time-shifted copy of the fixture was used to see rows); not validated on device.
- Next: 11 (X-ray 24 h plot), then 10 (NOAA R/S/G scales) and 9 (DST).

## 2026-10-14 (UTC): X-ray 24 h plot (backlog 11)

- Space weather card now plots the last 24 h of GOES 0.1-0.8 nm flux (10-minute peaks, log axis A1 to X10, B/C/M/X lines; line turns amber/red at M/X). Separate source `xrays-1-day.json` (~650 kB) polled every 15 min; the existing 6 h feed still drives the class value. Plot disappears if the 24 h data goes out of date.
- Compact card shows a thin plot without class letters; full card a taller one. Checked by offscreen screenshots at 800x480 and 1024x600 (light) and 1920x1080 (dark); not validated on device. The full (non-compact) layout was not rendered because all default panes are compact.
- Next: 10 (NOAA R/S/G scales), 9 (DST).

## 2026-10-14 (UTC): QA clipping fixes (#120, #121)

- Solar trend compact card uses short labels ("Sunspots", "SFI 30d") and the caption "Sep mean", so the month stays readable at 800x480.
- X-ray plot draws class letters only when line spacing is at least the font size (no more overprinting at 1920x1080); the full-size plot may take up to 160 px of free height.
- Checked by offscreen screenshots at 800x480 and 1920x1080 (dark); not validated on device. QML only, no model changes.
- Merged other areas' green PRs #124 and #125; #73 (docs) still conflicts and was left alone.
- Next: 10 (NOAA R/S/G scales), 9 (DST).

## 2026-10-14 (UTC): NOAA R/S/G scales (backlog 10)

- New `solarscales` widget ("NOAA scales"): SWPC `products/noaa-scales.json` polled every 10 min. Shows current R, S and G levels (coloured green/amber/red), and in the full card the 24 h maximum and tomorrow's SWPC outlook (R1-2 / R3+ and S1+ probabilities, expected G). Missing current levels or an unparseable body read as no data (loading/offline), never as zeros. Source terms recorded in the roadmap (same SWPC policy).
- Checked by offscreen screenshots at 800x480, 1024x600 (compact) and 1920x1080 (dark); the full non-compact layout was not rendered because all default panes are compact. Not validated on device. Fixture is a real capture (the feed is ~1 kB). ASan/UBSan run on the widget tests.
- #120 was fixed by #126 earlier today; the issue is still open for QA to close. Left #133 (not mine, iPad check still running) and #73 (docs, unknown mergeability) alone.
- Next: 9 (DST), then 7 (DRAP) / 8 (Aurora) with area G's map styles.

## 2026-10-14 (UTC): Dst index (backlog 9)

- New `solardst` widget ("Dst index"): SWPC `products/kyoto-dst.json` (hourly Kyoto Dst, ~7 days) polled every 30 min. Shows latest nT with a Quiet/Weak/Moderate/Intense/Super word, a 48 h plot with 0 and -50 nT guide lines, and in the full card the 24 h low and observed time. If the latest record is more than 6 h old the widget shows no data rather than an old value as current. Source terms recorded in the roadmap (same SWPC policy).
- Checked by offscreen screenshots at 800x480 and 1920x1080 (dark; fixture time-shifted to look current); 1024x600 rendered too. Full non-compact layout not rendered (all default panes are compact). Not validated on device. Fixture is a real capture.
- Merged #146 (docs, green). #147/#148 still had macOS/iPad checks running; #73 (docs) has a stale base and was left alone.
- Next: 12 (stats ranked by impact), then 7 (DRAP) / 8 (Aurora) with area G's map styles.

## 2026-10-14 (UTC): Impact ranking (backlog 12)

- New `solarimpact` widget ("Space weather impact"): the SWPC R/S/G levels (same `noaa-scales.json` feed, own 10-min poll) ranked worst first, ties broken by 24 h maximum, with a headline ("All quiet" or "G4 Severe: ...") and a one-line effect per level taken from the NOAA scale definitions. No data reads loading/offline, never "quiet".
- Scope is the R/S/G scales only; folding Kp, Dst and X-ray class into the ranking is left for later.
- Checked by offscreen screenshots at 800x480, 1024x600 and 1920x1080 (storm state from a synthetic fixture, screenshot only; the test fixture is the real quiet capture). Compact card shows level and word; the full layout was not rendered. Not validated on device.
- No open green PRs were mergeable this run (#161 and #163 had checks still running, #73 mergeability unknown); left alone.
- Next: 7 (DRAP) / 8 (Aurora) with area G's map styles.

## 2026-10-14 (UTC): Aurora pane and map style (backlog 8, part)

- New `solaraurora` widget ("Aurora"): SWPC OVATION nowcast `json/ovation_aurora_latest.json` (~900 kB, 1 degree grid, public, same SWPC terms) polled every 10 min. Card shows peak aurora probability north and south and the observation time. It publishes the `aurora` map style (360x180 image, green to red, legend "Aurora %", credit NOAA SWPC OVATION) and removes it when the grid is over 3 h old or the model goes away, so an old nowcast is never drawn as live.
- Fixture is a real capture trimmed to |lat| >= 55, even longitudes, non-zero cells (parser treats missing cells as 0). Tests cover parsing, hemisphere peaks, image orientation, staleness removal of the style; ASan/UBSan run on the widget test.
- Checked by offscreen screenshots at 800x480, 1024x600, 1920x1080 (dark; fixture time shifted to look current); the map style itself was not viewed on the map because style selection UI does not exist yet. Not validated on device.
- Shared file touched: one id added to the category map in `PaneChooser.qml`.
- No green PRs to merge this run (#171, #172 had checks running; #73 is not mine).
- Next: 7 (DRAP, `text/drap_global_frequencies.txt` reachable), wire auto Aurora/DRAP switch to live Kp/X-ray.

## 2026-10-14 (UTC): D-RAP pane and map style (backlog 7)

- New `solardrap` widget ("D-RAP"): SWPC `text/drap_global_frequencies.txt` (4 x 2 degree grid, ~42 kB, same SWPC terms recorded in the roadmap) polled every 10 min. Card shows the highest affected frequency with the X-ray/proton status and valid time. It publishes the `drap` map style (360x180, blue to red, legend "D-RAP MHz") and removes it when the table is over 2 h old; a truncated or malformed table is rejected, never shown as zeros.
- Fixture is the real capture (quiet, peak 4.0 MHz). Tests cover parsing, orientation/alpha of the image, staleness removal of the style; ASan/UBSan run on the widget test.
- Checked by offscreen screenshots at 800x480, 1024x600, 1920x1080 (dark, fixture time shifted); map style not viewed on the map (no selection UI yet). Not validated on device.
- Shared file touched: one id added to the category map in `PaneChooser.qml`.
- Merged #179 (hazards, green). #180 still had checks running; #73 mergeability unknown, left alone.
- Next: wire auto Aurora/DRAP switch to live Kp/X-ray; MUF style needs a terms-checked source.

## 2026-10-14 (UTC): Auto Aurora/DRAP switch wired (backlog 8)

- `spaceweather` now publishes its Kp and X-ray flux to the map styles framework (`set_auto_space_weather`) on every update. A value whose source is out of date is passed as unknown, so an old storm never keeps Aurora/DRAP on the map; the model clears auto mode when destroyed. Aurora shows for Kp >= 5, DRAP for M-class or stronger flares, when those styles are registered.
- Test: quiet fixture leaves the plain map, a Kp 6 body picks `aurora`, stale data picks nothing. No UI change, so no screenshots. Not validated on device.
- No mergeable PRs of mine; #73, #186 (macOS universal build failing) and #189 belong to others and were left alone. No open [B] issues.
- Next: 13-15 need source/terms decisions (VOACAP, HamQSL, tropo); MUF style needs a terms-checked source.

## 2026-10-14 (UTC): HamQSL conditions pane (backlog 14)

- New `solarhamqsl` widget ("HamQSL conditions"): N0NBH `solarxml.php` polled hourly (the author's request). Card shows the calculated HF day/night words for 80-40m, 30-20m, 17-15m and 12-10m, coloured Poor/Fair/Good, with the HamQSL.com (N0NBH) credit; the full layout adds the VHF openings. Hidden (never shown as current) when the report is over 6 h old; a truncated or malformed document is rejected. Terms recorded in the roadmap. The words are N0NBH's calculation, not ours (our own outlook stays in `bands`).
- Fixture is a real capture. Tests cover parsing, bad/truncated input, model live/stale/expired/offline. Checked by offscreen screenshots at 800x480, 1024x600 (compact; 1920x1080 rendered too); the full layout with VHF lines was not viewed. Not validated on device.
- Shared file touched: one id added to the category map in `PaneChooser.qml`.
- #73 (docs, not mine) has unknown mergeability and a stale base; left alone. No open [B] issues.
- Next: 13 (VOACAP-style) and 15 (tropo) need source/terms decisions.

## 2026-10-14 (UTC): Solar trend compact fit (backlog: compact layouts)

- Re-checked all space widgets in a pane at 800x480 and 1024x600 (dark): Space weather X-ray and Solar wind Bz now fit (earlier compact work). `solartrend` still lost its Kp bars: the fixed-height bar canvas was clipped at the pane bottom at 800x480, and also at 1280x800 and 1920x1080.
- `SolarTrendCard.qml`: compact slots drop the 30-day flux line (the metrics row already shows it) and keep the Kp bars; the bars now fill the height left in the pane (16 to 90 scaled px) instead of a fixed height. Checked by offscreen screenshots at 800x480, 1024x600 (light), 1280x800 and 1920x1080 (dark). QML only; full ctest passes (132/132). Not validated on device.
- Other auto-merge PRs: #73 has a merge conflict (not mine), #236 and #237 (other areas) had checks still running; left alone. No open [B] issues.
- Next: 13 (VOACAP-style) and 15 (tropo) need source/terms decisions; MUF style needs a terms-checked source.

## 2026-10-14 (UTC): Dst folded into the impact ranking (backlog 12)

- `solarimpact` now also polls `products/kyoto-dst.json` (30 min, same SWPC terms, already in the roadmap policy) and ranks the Kyoto Dst index as a fourth row "D" alongside R/S/G (levels: Quiet 0, Weak 1, Moderate 2, Intense 3, Super 5; 24 h low as the 24 h max). The row is dropped once the latest hourly value is over 6 h old, so an old storm never ranks as current. Kp and X-ray are not added separately: G and R already come from them.
- Tests: Dst level thresholds, quiet real fixture ranks last, synthetic intense storm ranks first with headline, stale drop; widget test passes under ASan/UBSan, full ctest green. No screenshots taken (offscreen platform unavailable for the app binary here; only a fourth 1.2-line row added, QML pane test passes). Not validated on device.
- No open [B] issues; #73 (not mine) and the other auto-merge PRs left alone.
- Next: 13 (VOACAP-style) and 15 (tropo) need source/terms decisions; MUF style needs a terms-checked source.

## 2026-10-14 (UTC): In-house path reliability model (backlog 13, first slice)

- Owner picked option C on #281 ("simple in-house, but not too simple"). New `bands/path_model` (pure C++, offline): great-circle path split into F2 hops of at most 3000 km, foF2/MUF from SFI and the sun's elevation at each hop midpoint, secant-law path MUF, P.533-style D-layer absorption (tuned x0.6), SNR in 2.5 kHz from power, antenna gain, slant free-space loss, ground-reflection loss and ITU-R P.372 rural noise, mode thresholds (SSB/CW/digital), Kp and polar-path penalty. Gives MUF, SNR, takeoff angle, hops and 0..1 reliability per band, plus a band x 24 h matrix. Documented as approximate in the header, not VOACAP.
- Tests (`path_model_regression`): London-New York distance, 20m open by day, 80m absorbed by day and better at night, 10m closed at night, MUF rises with SFI, Kp 7 cuts reliability, 20 dB less power is 20 dB less SNR, digital needs less SNR, NaN inputs stay finite, matrix shape. No UI yet, so no screenshots. Not validated on device.
- Next: matrix card (DE to DX timeline) and settings for power/mode/antenna, long-path option; MUF map style and tropo still wait on #281.

## 2026-10-14 (UTC): HF path reliability pane (backlog 13, second slice)

- New `bandsmatrix` widget ("HF path reliability"): runs `bands/path_model` for the DE to DX path over the next 24 UTC hours using SWPC SFI and Kp (same two products and cadence as `bands`). Card shows "Best now: band %" plus per-band percentages (compact), or a band x hour heat grid with an "approximate model" note (full layout). Shows "Set a DX target" when DX is unset or within 50 km of DE; recomputed when the UTC hour turns or the station changes; stale/offline states follow the indices' age. Fixed at SSB, 100 W for now; settings for power/mode/antenna and the MUF map are next.
- Tests: no station -> no matrix, station gives 8 bands x 24 cells, 20m open and 80m absorbed London-New York by day, hour roll, outage -> stale, DX == DE -> no path. Screenshots of the compact layout at 800x480, 1024x600 and 1920x1080 (dark); the full heat grid layout was not viewed (panes were compact at every size tried). Not validated on device.
- Shared file touched: one id added to the category map in `PaneChooser.qml`.
- No mergeable PRs: #286/#287 (others) had checks running, #73 (not mine) conflicts. No open [B] issues other than #281, already answered.
- Next: power/mode/antenna settings, long-path option; MUF map style and tropo wait on #281.

## 2026-10-14 (UTC): HF path reliability options (backlog 13, third slice)

- `bandsmatrix` reads optional `[bandsmatrix] mode = ssb|cw|digital` and `power_w` (1..1500) from the options file (same mechanism as the sensors offsets); the pane label shows the mode and power used. Unknown mode or out-of-range power falls back to SSB 100 W. Test covers CW 5 W and bad values. No QML change, so no screenshots; no in-app settings UI yet (needs a shared settings page). Not validated on device.
- Other open PRs (#73, #294, #295) belong to others; left alone. Only [B] issue is #281, already answered.
- Next: antenna gain option, long-path, in-app settings UI, MUF map style.
