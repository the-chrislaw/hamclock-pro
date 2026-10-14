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
