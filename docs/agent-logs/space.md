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
