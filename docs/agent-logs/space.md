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
