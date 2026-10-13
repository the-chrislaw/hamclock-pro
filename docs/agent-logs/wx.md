# Agent log: wx

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (UTC, hourly run)
- Merged #85 (beacons, area D; green). Left #73 (merge conflicts, not mine) and #60 (Apple jobs cancelled).
- E6 slice: weather card now shows humidity, sea-level pressure and cloud cover per end (optional fields, request adds `pressure_msl,cloud_cover`). Open-Meteo returned "daily limit exceeded" from the sandbox, so no new live fixture was captured; the new-field test uses a labelled synthetic document. Card not visually checked (no station in the screenshot harness). Next: pressure trend, DX time zone/city, new-DE/DX popup; then E3 events feeds (check terms).

## 2026-10-13 (UTC, second hourly run)
- #94 (area D) and #73 (area A) are not mine; #94 CI was still running, #73 left alone. No open `qa` issues.
- E5/E11 slice: new `sensors` widget, CPU temperature from sysfs thermal zones (prefers CPU-typed zone, validates range, reports unconfigured when absent). Tested against a fake thermal tree; not run on a Pi. Card is registered (pane chooser) but not on the default dashboard. Next: BME280 behind a platform interface, disk use, Wi-Fi RSSI; then weather pressure trend and E3 feeds (terms check).

## 2026-10-13 (UTC, third hourly run)
- #103 (area C, merge conflicts) and #104 (area D, CI running) are not mine; left alone. No open `qa` issues.
- E11 slice: sensors card adds Wi-Fi signal (/proc/net/wireless parse) and root-disk use (QStorageInfo); metrics hide when absent. Tested with fake files, release + ASan/UBSan. Card is not on the default dashboard so no screenshot shows it; not run on a Pi. Next: BME280 behind a platform interface, then pressure trend, E3 feeds (terms check).
