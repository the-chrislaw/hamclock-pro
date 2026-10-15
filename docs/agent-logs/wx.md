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

## 2026-10-14 (UTC, hourly run)
- #112 (area D) and #73 (area A) are green/old but have merge conflicts and are not mine; left alone. No open `qa` issues.
- E6 slice: pressure trend. Request adds `hourly=pressure_msl&past_hours=3&forecast_hours=1`; weather card shows "1015 hPa falling/rising/steady" (±1 hPa over 3 h). No trend when the hourly series is missing or has nulls. Trend test uses labelled synthetic documents (no new live fixture; Open-Meteo quota was exhausted last run). Card not visually checked (no station in the screenshot harness). Next: DX time zone/city, new-DE/DX popup, BME280 behind an interface, E3 feeds (terms check).

## 2026-10-14 (UTC, second run)
- Merged #128 (sunmoon, area D; green). #73 (area A) left alone. 
- Fixed qa #117: weather card columns now share the width equally and the compact layout shows temperature + short summary only (wind/humidity/air dropped first). Rendered at 800x480, 1024x600 and 1920x1080 dark with a station at the fixture position; columns no longer collide. Not validated on-device. Next: DX time zone/city, new-DE/DX popup, BME280 behind an interface, E3 feeds (terms check).

## 2026-10-14 (UTC, third run)
- #137 (area D) CI still running, #73 (area A) conflicting: neither mine, left alone. No open `qa` issues for E.
- E11 slice: BME280/BMP280 on the sensors card via the kernel IIO driver (sysfs; temp, pressure, humidity, dew point by Magnus formula). Absent hardware hides the metrics. Tested against a fake IIO tree, release build; not run on a Pi. No calibration offsets or history yet. Next: DX time zone/city, new-DE/DX popup, E3 feeds (terms check).

## 2026-10-14 (UTC, fourth run)
- Merged #154 (docs, green). #156/#158 CI was still running, #73 conflicting: not mine. No open `qa` issues for E.
- E5/E12 slice: new read-only `gps` widget, gpsd JSON client on localhost:2947 (fix quality, position, grid, satellites used). No gpsd -> "unconfigured"; retries every 15 s; no fix is never shown as live. Does not set DE and does not use GPS time yet. Tested against a loopback fake gpsd (release + ASan/UBSan); not run with a real receiver. Not on the default dashboard so no screenshot. Next: use fix for DE (needs area A setting), NTP choice, DX time zone/city, E3 feeds (terms check).

## 2026-10-14 (UTC, fifth run)
- #161 and #165 (areas G/D) still had CI running; #73 (area A) conflicting: not mine, left alone. No open `qa` issues for E.
- E4 slice: new read-only `radio` widget, rigctld client on localhost:4532 polling only `f` and `m` (test asserts nothing else is sent). Shows frequency, band, mode; unconfigured without rigctld; RPRT errors are not live. Tested against a loopback fake rigctld (release + ASan/UBSan); not run with a real rig. Not on the default dashboard so no screenshot. Next: flrig, header display, DX time zone/city, E3 feeds (terms check).

## 2026-10-14 (UTC, sixth run)
- #175 (area D) had both Linux checks green but Apple jobs still running; #73 (area A) conflicting: neither mine, left alone. No open `qa` issues for E.
- E10 slice: new read-only `rotator` widget, rotctld client on localhost:4533 sending only `p` (test asserts nothing else is sent). Azimuth/elevation; unconfigured without rotctld; RPRT errors and out-of-range values are not live. Loopback fake rotctld test; not run with a real rotator. No move/track commands (safety review pending). Not on the default dashboard so no screenshot. Next: flrig, DX time zone/city, E3 feeds (terms check).

## 2026-10-14 (UTC, seventh run)
- #73 (area A, conflicting), #182/#183/#184 (areas tvOS/D/C) had CI still running: not mine, left alone. No open `qa` issues for E.
- E6 slice: DX/DE local time. Weather request now uses `timezone=auto`; the parsed `utc_offset_seconds` gives "14:32 (UTC-7)" on each end of the weather card (hidden if the service omits it). City name and a new-DE/DX popup are still pending. Offline tests incl. offset formatting; release + ASan/UBSan weather tests. Card not visually checked (no station in the screenshot harness); not validated on-device. Next: flrig, DX city/popup, E3 feeds (terms check).

## 2026-10-14 (UTC, eighth run)
- #186 (area F, a Linux-green but macOS universal job failed), #191 (area D, Apple jobs running), #73 (area A, conflicting): not mine, left alone. No open `qa` issues for E.
- E12 slice: clock sync status. New `src/platform/ntp_status` (read-only `chronyc tracking`, falling back to `timedatectl show -p NTPSynchronized`; no network, no clock changes) shown as a "Clock" metric on the sensors card ("Synced, 12 ms" / "Not synchronised"; hidden where neither tool exists). Parsers tested on sample text; release + ASan/UBSan sensors tests. Not run on a Pi. NTP server choice and GPS time are still pending. Next: DX city/popup, flrig, E3 feeds (terms check).

## 2026-10-14 (UTC, ninth run)
- #208 (area C, macOS/iPad jobs failing or running), #207 (area D, no checks yet), #73 (area A): not mine, left alone. No open `qa` issues for E.
- E14 slice: `src/platform/gpio` output interface (`GpioOutputs`, BCM line parsing 2..27, Null default backend that drives nothing, in-memory Fake for tests). No hardware backend yet (needs on-device testing); nothing consumes it yet. Offline test; release + ASan/UBSan. Next: gpiod backend, ON AIR/alarm wiring, DX city/popup, flrig.

## 2026-10-14 (UTC, tenth run)
- Merged #214 (dxcluster, green). #207 (area D) CI running, #73 (area A) conflicting: not mine. No open `qa` issues for E.
- E4 slice: read-only flrig fallback for the radio card. XML-RPC to localhost:12345 asking only `rig.get_vfo` and `rig.get_mode`; used only while rigctld is not connected (rigctld wins), retries every 15 s, faults/garbage are never shown as live. Parser tests plus a loopback fake flrig HTTP server; release + ASan/UBSan radio tests. Not run with real flrig. Next: header display, DX city/popup, gpiod backend, E3 feeds (owner input #200).

## 2026-10-14 (UTC, eleventh run)
- #218/#219 (areas C/D) CI still running, #73 (area A) conflicting: not mine, left alone. No open `qa` issues for E.
- E2 slice: lookup card shows "5210 km · 52° SP" (distance and short-path bearing from DE to the centre of the returned grid; hidden without a DE or a valid grid). Uses the existing geometry/locator helpers. Offline tests for malformed grids and a known cell; full suite passes (release). ASan/UBSan and screenshots were not run locally (card needs an account, not in the screenshot harness). Spotted-call wiring still pending (needs area C selection). Next: header radio display, DX city/popup, gpiod backend.

## 2026-10-14 (UTC, twelfth run)
- #232 (area C) had both Linux checks green but Apple jobs running; #73 (area A) conflicting: not mine, left alone. No open `qa` issues for E.
- E11 slice: sensors card shows a pressure trend (Rising/Falling/Steady) under the BME280 pressure, from an in-memory history (one sample/min, 3 h window, needs 30 min, scaled to a 3 h change, steady within +-1 hPa). Hidden without history or sensor. Unit tests for the history; full suite passes (release build); ASan/UBSan left to CI; no screenshot (card needs hardware). Not run on a Pi. Next: calibration offsets (needs a setting), DX city/popup, gpiod backend, header radio display.

## 2026-10-14 (UTC, thirteenth run)
- Merged #238 (area B, green). #239/#240 (areas C/D) CI running, #73 (area A) conflicting: not mine. No open `qa` issues for E.
- E11 slice: BME280 corrections. `[sensors] temp_offset_c` and `pressure_offset_hpa` in options.ini are added to the readings (dew point and trend follow the corrected values); empty, garbage or implausible offsets (>20 C, >50 hPa) are ignored. No Settings control yet (area A page). Unit tests plus a model test with a temp options.ini; release full suite and ASan/UBSan sensors test pass. No screenshot (card needs hardware); not run on a Pi. Next: DX city/popup (needs a reverse-geocode source and terms check), gpiod backend, header radio display (area A).


## 2026-10-14 (UTC, fourteenth run)
- Merged #245 (docs, green). #242 (no checks yet), #248/#250 (areas tvOS/D), #73 (area A, conflicting): not mine. No open `qa` issues for E.
- E15 slice: new read-only `adsb` card polling the local PiAware/dump1090 `aircraft.json` (localhost:8080, GET only, 5 s): aircraft heard in the last 60 s and how many carry a position. No receiver -> "unconfigured"; receiver clock >60 s from ours -> stale; garbage -> error. Loopback fake HTTP server test; not run against a real receiver. No settings (host/port) or map overlay yet (needs area B/A). Not on the default dashboard so no screenshot. Next: DX city/popup (reverse-geocode terms), gpiod backend, header radio display.

## 2026-10-14 (UTC, fifteenth run)
- #256/#257 (areas C/D) CI still running, #73 (area A) conflicting: not mine, left alone. No open `qa` issues for E.
- E6 slice: weather card shows today's "H 18 / L 9 °C" (Open-Meteo `daily` max/min, forecast_days=1, same request; hidden if absent, null or low > high). Offline parser tests; full suite passes (release). ASan/UBSan left to CI; no screenshot (no station in harness); not validated on-device. Next: DX city/popup (reverse-geocode terms), gpiod backend, header radio display.

## 2026-10-14 (UTC, sixteenth run)
- #265/#266 (areas C/D) CI still running, #73 (area A) conflicting: not mine, left alone. No open `qa` issues for E.
- E12 slice: `gps_nmea` parser for serial GPS receivers (GGA position/fix/satellites, RMC validity and UTC time; checksum required, void fixes, bad hemispheres and out-of-range values rejected). Offline tests on reference sentences; debug suite for gps passes, ASan/UBSan left to CI. Parser only: no serial reader (QtSerialPort availability to check), no DE or time use. Not run with a real receiver. Next: serial reader, DX city/popup (reverse-geocode terms), gpiod backend.

## 2026-10-14 (UTC, seventeenth run)
- #274 (tvOS) and #73 (area A) open, not mine, left alone. No open `qa` issues for E.
- E12 slice: NMEA stream layer for the future serial GPS reader. `NmeaStream` splits raw bytes into lines (partial lines held, >128-byte lines dropped); `NmeaTracker` merges GGA+RMC, reports a position/UTC only while younger than 5 s, clears on a no-fix GGA. Offline tests; full release suite and ASan/UBSan gps test pass. Still no serial reader (QtSerialPort not installed here) and no DE/time use. Not run with a real receiver. Next: serial reader, DX city/popup (reverse-geocode terms), gpiod backend.

## 2026-10-14 (UTC, eighteenth run)
- No open PRs of mine; #289/#290 (areas B/D) had green Linux checks but Apple jobs still running, #73 (area A) open: not mine, left alone. No open `qa` issues for E.
- E12 slice: GPS card compares the gpsd TPV `time` with the system clock and shows "Clock agrees with GPS (within 2 s)" or "Clock is N s ahead of/behind GPS" (full-size layout only; differences under 2 s are never shown as a precise offset because gpsd reports late). Display only: it never sets the clock or DE. Offline parser/text tests; full release suite passes, ASan/UBSan left to CI; no screenshot (card needs a fix). Not run with a real receiver. Next: serial reader, DX city/popup (reverse-geocode terms), gpiod backend.

## 2026-10-14 (UTC, nineteenth run)
- #297/#298/#299 (areas tvOS/C/D) have Apple jobs still queued/running, #73 (area A) conflicting: not mine, left alone. No open `qa` issues for E.
- E4 slice: rigctld and rotctld host/port are read from options.ini (`[radio]`/`[rotator]` `host`, `port`); invalid hosts (anything but letters, digits, `.`, `-`, `:`) and ports outside 1..65535 fall back to the defaults field by field. Remote LAN hosts now possible; still strictly read-only. flrig stays on localhost. Offline parser tests; no Settings control yet (area A page). Next: serial GPS reader, DX city/popup (reverse-geocode terms), gpiod backend, header radio display.

## 2026-10-14 (UTC, twentieth run)
- #306/#307/#73 are other areas' PRs with Apple jobs queued or conflicts: not mine, left alone. No open `qa` issues for E.
- E12 slice (NTP): the sensors card's clock line now also shows chrony's stratum and reference source ("Synced, 3 ms, stratum 2 via time.example"). The source name is shown only if it is plain host/address characters (<= 64), otherwise dropped. Display only, read-only `chronyc tracking`; no source selection yet (would need privileged config, out of scope). Tests updated and extended; full release suite and ASan/UBSan sensors tests pass. Not run on a Pi. Next: serial GPS reader, DX city/popup (reverse-geocode terms), gpiod backend.

## 2026-10-14 (UTC, twenty-first run)
- #73 (area A) open and conflicting: not mine, left alone. No open `qa` issues for E.
- E3 paused: NG3K ADXO is copyrighted with no reuse terms (same as the parked contest calendar); opened "Owner input needed: DXpedition calendar source (E3)" (#315) with options.
- E6 slice: weather card labels now read "DE · San Francisco, US" / "DX · ...", from OSM Nominatim reverse (zoom 10). Policy recorded in the roadmap: one request per end on position change, delayed 1.2 s / 2.4 s so DX bursts collapse and ends never coincide (<1 req/s), weekly refresh, disk cache; status line credits OpenStreetMap contributors. Real reply fixture, offline parser/URL tests, model test with a fake transport; full release suite and ASan/UBSan weather test pass. No screenshot (needs a station, not in the harness); not run on a device. Next: DX city popup on new DE/DX (area G wiring), serial GPS reader, gpiod backend.

## 2026-10-15 (UTC, twenty-second run)
- Merged #320 (tvOS, green). #321/#322 (areas C/D) CI still running, #73 (area A) conflicting: not mine. Branch deletion of the merged head was denied by the sandbox, left in place. No open `qa` issues for E.
- E11 slice: sensors card CPU caption adds "Peak N °C (3 h)" (in-memory, one sample/min, 3 h window; full layout only). Unit tests for the window; full release suite passes, ASan/UBSan left to CI; no screenshot (card shows only on hardware with a thermal zone). Not run on a Pi. Next: serial GPS reader, DX city popup (area G wiring), gpiod backend.

## 2026-10-15 (UTC, twenty-third run)
- #73 (area A) open and conflicting: not mine, left alone. Took open QA issue #326.
- Fix: `Metric.qml` label now fills its column and elides on the right, so long "DE · City, CC" headings no longer run under the neighbouring weather label. One shared-file change (area A), two lines, backward compatible (short labels render the same). Full release suite passes; ASan/UBSan left to CI. No screenshot with a long city (harness has no station), not validated on-device.

## 2026-10-15 (UTC, twenty-fourth run)
- #334 (area D) CI still running, #73 (area A) conflicting: not mine, left alone. No open `qa` issues for E.
- E11 slice: sensors card Wi-Fi caption adds "Weakest N dBm (3 h)" (in-memory, one sample/min, full layout only), via `PeakHistory::trough()`. Unit test added; full release suite passes, ASan/UBSan left to CI; no screenshot (card shows only on hardware with Wi-Fi). Not run on a Pi. Next: serial GPS reader, DX city popup (area G wiring), gpiod backend.

## 2026-10-15 (UTC, twenty-fifth run)
- Merged #344 (area C, green). #345 (area D) CI running, #73 (area A) conflicting: not mine. No open `qa` issues for E.
- E6 slice: weather card air line adds "dew N °C", computed locally from temperature and humidity (Magnus formula, no extra request; hidden for 0 % RH or out-of-range temperatures). Offline tests against reference values; ASan/UBSan left to CI; no screenshot (no station in harness); not validated on-device. Next: serial GPS reader, DX city popup (area G wiring), gpiod backend.

## 2026-10-15 (UTC, twenty-sixth run)
- #350 (area D) had no checks yet, #73 (area A) conflicting: not mine, left alone. No open `qa` issues for E.
- E11 slice: sensors card Disk caption adds free space ("12.4 GB free" / "850 MB free", full layout only), via `format_free_bytes`. Unit tests for the formatting; ASan/UBSan left to CI; no screenshot (card shows only on hardware with sensors); not run on a Pi. Next: serial GPS reader, DX city popup (area G wiring), gpiod backend.

## 2026-10-15 (UTC, twenty-seventh run)
- #356 (area C) macOS job still running, #73 (area A) conflicting: not mine, left alone. No open `qa` issues for E.
- E4 slice: radio card Mode caption shows the passband width from rigctld's `m` reply ("2.4 kHz", "500 Hz"); zero/invalid widths hidden, full layout only. Offline parser tests; radio tests pass locally, full suite and ASan/UBSan left to CI; no screenshot (card needs a rig); not run with a real rig. Read-only. Next: serial GPS reader, DX city popup (area G wiring), gpiod backend.
