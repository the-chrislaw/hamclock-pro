# HamClock Pro roadmap

Written October 12, 2026. This is the single plan from today's state to a 1.0 release. It replaces no evidence document; it orders the work. Detailed specs, audits and validation notes stay in `docs/` and `docs/workstreams/`.

## Scope change (October 13)

After the first Pi run the owner judged the scope too narrow and the Qt Widgets look dated. Decisions: **every widget works with live data**, the UI moves to **Qt Quick (QML)**, and work runs as **parallel agents, one per widget area**. The widget contract and the full backlog by area are in [docs/WIDGETS.md](WIDGETS.md), which now drives the work; the milestones below remain for platform, packaging and release.

## Where we are

- **Code:** C++17, Qt 6 only (the SDL2 experiment was retired October 12). 33 CTest suites pass on macOS and in CI on Ubuntu.
- **Branches:** the 35-PR stack was merged into `main` via #36 on October 12 (M0).
- **CI:** GitHub Actions builds and tests every PR on Ubuntu, including an ASan/UBSan job. The repository went **public on October 13** (free Actions minutes); `main` is protected and requires both CI checks before merge.
- **Releases:** every merge to `main` publishes an arm64 Bookworm `.deb` and a macOS app as the rolling `latest` pre-release.
- **Hardware:** first real run on a Pi 4 + 7-inch 1024×600 screen on Oct 13: installs and runs at 0.6 % CPU / 84 MB; see `docs/hardware-validation.md`. Owner reference setups: Pi 4 + generic 7-inch HDMI touchscreen, and a TV over HDMI.
- **Design:** the approved look is `design/dashboard.html` and the refined mockups; the native Qt dashboard does not match it yet (`docs/native-approved-design-gap.md`).

## Definition of 1.0

HamClock Pro installs with one command on any supported Raspberry Pi (see "Platform targets"), boots straight into the dashboard on any display from a 7-inch touchscreen to a 4K TV, and shows, with real data and no simulated readings:

- UTC and DE local time, DE/DX stations with locator, distance and bearing
- the world map with land fill, day/night terminator and the DE→DX path
- every widget in [docs/WIDGETS.md](WIDGETS.md) working with live data (space weather and solar, spots, astronomy and satellites, weather, lookup, events, radio)
- a settings screen that saves and restores everything above
- the approved visual design in light and dark, scaling cleanly to the screen, usable by touch alone or by keyboard/remote alone

It must run both inside a desktop session and directly on the display without a desktop (Pi OS Lite), and run acceptably on a Pi 3 and Zero 2 W. 

## Milestones

Sizes are relative effort: S = days, M = 1–2 weeks, L = several weeks. They assume one main developer with AI agents.

### M0: Consolidate (S) — done October 12

Get one healthy `main` before writing more features.

1. Review the stack bottom-up and merge #1 → #35 in order, then the `wip/` branch. Squash-merge is fine; keep each PR's title as the commit.
2. Add GitHub Actions: Ubuntu + Qt 6 build, `ctest`, and an ASan/UBSan job. Add an arm64 build job to catch Pi compile problems early.
3. Delete merged branches. From now on, feature PRs target `main` directly; no stacks more than 2 deep.
4. Close or merge the Codex workspace's uncommitted copy so there's one source of truth (see "Working agreements").

**Done when:** `main` builds green in CI with all 30 suites, and there are no open stacked drafts.

### M1: Finish the station clock (M)

Turn the pieces that already exist into one working screen.

- Wire the DE clock settings dialog into Big Clock → Options. Apply the two corrections in `docs/workstreams/de-native-integration-plan.md`. Fix the disabled offset field not looking disabled.
- DE local time with real time zones and daylight saving (P1 in `docs/parity-next.md`). Use the OS tz database through Qt's `QTimeZone`; don't hand-roll DST rules. **Slice 1 done:** `src/qt/de_time_zone` resolves an IANA zone's offset at a UTC instant (tested across US DST changes). **Slice 2 done:** `adapt_de_clock_zone_civil` turns an IANA zone + UTC instant into DE civil fields (tests across the New York DST changes). **Slice 3 done:** the DE configuration carries an optional IANA zone id (Automatic mode) and the display snapshot uses it for DST-aware DE civil time. **Slice 4 done:** the DE settings dialog has an IANA zone id field (Automatic mode; unknown ids are rejected). **Slice 5 done:** the station settings file stores an optional `de_zone` (backward compatible; the station editor preserves it). **Slice 6 done:** `de_clock_configuration_for_saved_zone` turns the saved `de_zone` into the startup DE configuration (tested). Still to do: use it in the dashboard (show DE local time) and the Big Clock -> Options entry.
- Day/night terminator and subsolar point on the map (computed offline; no data feed needed). **Done:** `src/domain/solar_position` (tested against Meeus) and the night shade + subsolar marker on the map, refreshed once a minute from the OS UTC clock.
- ~~Retire the SDL2 executable~~ Done: CLI tools moved to `hamclock-pro-cli`, all checks in CTest.
- **Go public (owner decision, Oct 12):** at the end of M1, run an open-source sanitizer pass (secrets, personal paths, internal notes), add an "independent project, not affiliated with the original HamClock" notice to the README, then the owner flips the repo to public. After that, enable branch protection requiring CI on `main` and switch Pi builds to native Arm runners.

**Done when:** someone can set their station, see correct UTC and local time across a DST change, and watch the terminator move, on a Mac.

### M2: Approved design in native Qt (M)

Follow `docs/native-approved-design-gap.md`: station-first left card, three instrument panes plus space-weather strip, rounded map card, semantic light/dark tokens, fixed type scale. Panes without data show a clear "not available yet" state.

- **Resolution independence:** layout and type scale from 800×480 up to 3840×2160 with no fixed pixel layouts; also check common small panels (1024×600) and 720p/1080p TVs.
- **TV mode:** large type, overscan-safe margins (configurable inset), and every function reachable by keyboard or remote (arrow keys, Enter, Back/Escape); touch is optional, never required.

**Done when:** side-by-side screenshots of native vs. the approved mockup at 800×480, 1024×600, 1280×800 and 1920×1080 are signed off by the owner, and a full keyboard-only pass succeeds.

### M3: Runs on real Pis and screens (M)

- **Builds:** add a 32-bit `armhf` `.deb` (Bookworm) next to the arm64 one in the release workflow; keep a best-effort ARMv6 build for Pi 1 / Zero W.
- **Two run modes:** inside a desktop session (Wayland/X11), and kiosk without a desktop via Qt's `eglfs` (fallback `linuxfb`) platform, launched at boot by a systemd service. Document both.
- **Reference setups (owner):** Pi 4 + generic 7-inch HDMI touchscreen; Pi 4 + TV over HDMI. Touch input via evdev/libinput in kiosk mode; check touch calibration on generic panels.
- Measure CPU, memory, startup time and frame rate on each setup; record in `docs/hardware-validation.md`. Repeat on a Pi 3 and Zero 2 W when available (community testers welcome).
- Touch-only pass (every control ≥ 44 px) and remote/keyboard-only pass; decide whether an on-screen keyboard is needed for callsign entry.
- From the first Pi run: start fullscreen by default on small screens (and a `--fullscreen`/`--windowed` option), and let the map use the full card width instead of letterboxing at 1024×600.

**Done when:** the owner uses it for a day on both reference setups without a crash, and the 64-bit and 32-bit packages install cleanly on a fresh Pi OS image.

### M4: Live data (L)

Source policy is decided (see "Data source policy"); M4 is unblocked. Each further feed needs the same recorded check of terms and polling rate before coding.

1. **Space weather:** NOAA SWPC JSON for Kp (parser exists), solar flux, X-ray. Cache to disk; show age and an offline state.
2. **DX cluster:** telnet client to a user-entered node with callsign login; no default node, show "Not connected" until configured. Setup may list a few well-known public nodes as suggestions. Feed the existing `spot_store`. Plot spots on the map.
3. **Data plumbing shared by both:** one fetch scheduler, backoff, timeouts, and stale/error/offline states in the UI.

**Done when:** a 24-hour soak test on the Pi with Wi-Fi drops shows correct recovery and no memory growth.

### M5: 1.0 release (M)

- **Two install options** (owner decision, Oct 13):
  - **Ready-made SD card image** for most users: Raspberry Pi OS Lite with HamClock Pro preinstalled, booting straight into the dashboard in kiosk mode (no desktop). Built in CI with Raspberry Pi's `pi-gen`, published per release for 64-bit and 32-bit, flashable with Raspberry Pi Imager. Needs an on-screen first-run setup (Wi-Fi, callsign, location, time zone) and a self-update path.
  - **Package install** for existing Pi OS systems: one-command install/update (apt repository or `.deb` + script) for arm64 and armhf; desktop or kiosk mode; uninstall leaves no system changes.
- Measurement pass across the supported tier; fix or document limits for best-effort boards.
- Third-party notices (Qt LGPL via dynamic linking, Natural Earth, data sources). Name stays "HamClock Pro" with the independence notice.
- User guide: install, first-run setup, desktop vs. kiosk mode, TV setup (overscan, remote), troubleshooting.
- Tag `v1.0.0`.

## Apple platforms (in parallel with the Pi work)

Owner decision (Oct 13): a **macOS app** and an **iPad app** from the same Qt Quick code. Tracked as area F in [docs/WIDGETS.md](WIDGETS.md). Signing, notarization and App Store/TestFlight need an Apple Developer account (owner, $99/yr); until then the macOS app ships unsigned and the iPad app is built for the Simulator in CI. An iPhone layout is deferred until users ask for it.

## After 1.0

Rough order, highest value first. Each item is one PR-sized feature or a short series.

| Order | Feature (4.22 family) | Notes |
|---|---|---|
| 1 | Sun & Moon panes (F04) | Offline astronomy; no data permissions needed |
| 2 | Live spots via PSKReporter (F14) | Check PSKReporter usage terms |
| 3 | Satellite predictions (F05) | Needs TLE source (CelesTrak) and SGP4 library choice |
| 4 | Band conditions / propagation (F07) | Online VOACAP needs prior agreement; consider an offline model |
| 5 | NCDXF beacons (F19) | Offline schedule |
| 6 | Map projections, grid/zones/cities overlays (F11, F12) | Data licensing per dataset |
| 7 | Contests & DXpeditions (F17), On-the-air (F18) | Feed terms per source |
| 8 | DE/DX weather (F10), solar imagery (F09) | API keys / image terms |
| 9 | Radio & rotator control (F20) | Hamlib; requires safety review |
| 10 | GPS (F21), sensors & CPU temp (F22) | Pi hardware specific |
| 11 | Brightness & display schedule (F27) | Pi display backlight control |
| 12 | Browser view & REST API (F28) | Security review required before any network listener |
| 13 | ADIF import (F15), watch lists (F16), news feeds (F24) | |
| 14 | Saved configurations, pane rotation (F25, F26), diagnostics/updates (F29) | |
| 15 | EME planning (F06) | |

## Platform targets

Owner decision (Oct 12): it should run on every Raspberry Pi model and any screen; many users run it on a TV. Support is tiered by how it is tested, not by what is allowed.

| Tier | Boards / OS | Commitment |
|---|---|---|
| Supported | Pi 5, Pi 4, Pi 3B/3B+, Zero 2 W on 64-bit Raspberry Pi OS (Bookworm) | Built and tested for every release |
| Supported (32-bit) | Same boards on 32-bit Raspberry Pi OS | `armhf` package from the same code |
| Best effort | Pi 1, Pi Zero W (ARMv6, 512 MB) | Built; may be slow; community-tested |

Displays: any HDMI or DSI display from 800×480 to 4K; touch optional; keyboard/remote always sufficient. Run modes: desktop session or kiosk (`eglfs`/`linuxfb`) on Pi OS Lite. Development builds for macOS and desktop Linux continue.

## Data source policy

Note from the feature audit (Oct 13): the original gets almost all feeds pre-processed from its author's backend server (map images for DRAP/MUF/VOACAP/clouds, contests, DXpeditions, cities, hazards). HamClock Pro does not use that server; every such feature needs a direct, terms-checked public source (e.g. NOAA SWPC products, NASA GIBS imagery) or an offline open-source model (for VOACAP-style predictions). Record each choice here before coding.


Owner decision (Oct 12):

- **Space weather:** NOAA SWPC JSON products (US government public data, no key). Each clock fetches directly from the source; there is no project-run relay server. Polite polling: Kp and X-ray every 5–15 minutes, solar flux a few times a day. Cache to disk, show data age, and show an offline state rather than stale-as-live or invented values. Alerts, watches and warnings (`products/alerts.json`, same terms) are polled every 10 minutes; only messages still in force are shown. R/S/G scales (`products/noaa-scales.json`, same terms) are polled every 10 minutes. The Dst index (`products/kyoto-dst.json`, Kyoto WDC data republished by SWPC, same terms) is polled every 30 minutes. D-RAP (`text/drap_global_frequencies.txt`, same terms, ~42 kB) is polled every 10 minutes. **HamQSL / N0NBH** (`https://www.hamqsl.com/solarxml.php`, ~2 kB XML, free to use; the author asks for at most one fetch per hour and that credit to HamQSL.com and N0NBH is kept, so the pane polls hourly and always shows the credit; the VHF items there are third-party data used with N0NBH's permission, shown only as relayed). Data is hidden once over 6 h old.
- **DX cluster:** no default node. The user enters node and callsign; "Not connected" until then.
- **Later feeds** (PSKReporter, CelesTrak TLEs, propagation, weather, imagery): check terms of use and polling limits and record them here before coding. Online VOACAP requires a prior agreement.
- **Satellites:** CelesTrak `celestrak.org/NORAD/elements/gp.php?GROUP=amateur&FORMAT=tle` (public GP data, no key). CelesTrak asks for at most one download per group per 2 hours; we fetch once a day and cache to disk, and keep the last elements when offline. Passes are computed offline with the vendored SGP4 (licence in docs/third-party-sgp4.md).
- **Satellite transmitters:** SatNOGS DB `https://db.satnogs.org/api/transmitters/?format=json` (open data, CC BY-SA 4.0, no key; attribution "frequency data from SatNOGS DB" required wherever shown). Planned poll: at most once a day, cached to disk, honest User-Agent. Reachable from the sandbox; fetched daily by the Satellites model as `...?format=json&service=Amateur&alive=true` (about 330 kB; the whole database is 4 MB).
- **WSPR spots:** wspr.live ClickHouse HTTP endpoint (`https://db1.wspr.live/?query=...FORMAT JSON`, WSPRnet spot database mirror, no key). Reachable from the sandbox (Oct 15). Terms: the site asks for fair use and lists no hard limit; we plan one query per 5 minutes, `LIMIT 100`, last hour, honest User-Agent, only when a callsign is set. Owner to confirm the terms before the pane polls it; the parser (`psk::parse_wspr`) exists but nothing fetches yet.
- **Planets:** no network. Positions come from the JPL "Keplerian Elements for Approximate Positions of the Major Planets" (E.M. Standish, Solar System Dynamics group, valid 1800-2050; published for free use, https://ssd.jpl.nasa.gov/planets/approx_pos.html), implemented in `src/domain/astro_planets.*`. Accuracy is arcminutes to a fraction of a degree; the tests check oppositions, conjunctions and elongations against almanac dates.
- **Weather:** Open-Meteo `api.open-meteo.com/v1/forecast` (no key). Free for non-commercial use under CC BY 4.0: show "Weather by Open-Meteo.com" (the card status line does). Limits: 10,000 calls/day; we poll two points every 15 min (192/day). Coordinates are sent rounded to 0.01°. Terms: https://open-meteo.com/en/terms. If the project ever charges for the app, a commercial plan or key is needed: owner decision.
- **Place names (weather card):** OSM Nominatim `nominatim.openstreetmap.org/reverse` (zoom 10). Usage policy: max 1 request/s, identifying User-Agent (we send HamClock Pro's), cache results, no bulk use; ODbL data, "© OpenStreetMap contributors". We make one request per end (DE, DX) when the position changes, delayed 1.2 s / 2.4 s so bursts collapse, refreshed weekly, cached on disk. The card status line credits both ("Weather by Open-Meteo.com · places © OpenStreetMap contributors").
- **DXCC prefix table:** AD1C `cty.dat` (country-files.com, MIT licence), bundled in the app as a snapshot and parsed offline; no run-time fetch. Notice in docs/third-party-cty.md.
- **Callsign lookup:** QRZ.com XML (`xmldata.qrz.com`, needs the user's own account; full data needs an XML subscription; session key is valid ~24 h, we re-login once on expiry) or HamQTH (`hamqth.com/xml.php`, free account). Only user-typed calls are looked up, once per call per run (in-memory cache), never in bulk and never scraped. Credentials stay C++-only in the owner-only credentials file; URLs are never logged or shown. Terms: https://www.qrz.com/XML/current_spec.html and https://www.hamqth.com/developers.php.
- **Earthquakes:** USGS GeoJSON summary feed `earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_day.geojson` (M2.5+, past day). US-government public domain, no key; credit USGS. Feed is rebuilt every minute and USGS asks clients not to poll faster than that; we poll every 5 min. Terms: https://www.usgs.gov/information-policies-and-instructions/copyrights-and-credits and https://earthquake.usgs.gov/earthquakes/feed/v1.0/geojson.php. The widget shows the feed build time as data age.
- **Solar imagery:** NASA SDO latest images `sdo.gsfc.nasa.gov/assets/img/latest/latest_512_{0193,0304,HMIIC}.jpg` (512 px). NASA imagery is public domain; credit "NASA/SDO and the AIA, EVE and HMI science teams" (https://sdo.gsfc.nasa.gov/data/ and https://www.nasa.gov/nasa-brand-center/images-and-media/). The assets refresh about every 15 minutes, so only the shown channel is polled, every 15 min, with an honest User-Agent. The card shows download age (the image itself is typically ~15 min older) and rejects bodies that do not decode as an image. Reachable from the sandbox: not checked; the fixture is a one-time capture.
- **Tropical storms:** NHC active-storm index `www.nhc.noaa.gov/CurrentStorms.json` (Atlantic and East/Central Pacific; JTWC for the West Pacific is not covered yet). NOAA/NWS government data, public domain, no key; credit NHC (https://www.weather.gov/disclaimer, https://www.nhc.noaa.gov/about.shtml). The index changes only with advisories (every 3-6 h), so we poll every 30 min with an honest User-Agent. The widget shows the advisory issuance time. Checked reachable from the sandbox on 2026-10-13. Forecast positions come from the same host's forecast/advisory text (`www.nhc.noaa.gov/text/MIATCM<basin><n>.shtml`, the URL given in the index; public domain), fetched at the same 30 min cadence for the three strongest storms only, and used only when its advisory number matches the index and it is fresh. (The KMZ/shapefile tracks need a zip reader and are not used.)
- **NWS marine warnings:** `api.weather.gov/alerts/active?message_type=alert&event=...` (Gale, Storm, Hurricane Force Wind, Special Marine, Hazardous Seas warnings; Small Craft, Brisk Wind, Low Water advisories). NOAA/NWS public domain, no key; credit NWS (https://www.weather.gov/disclaimer). api.weather.gov requires an identifying User-Agent (sent) and no hammering; we poll every 10 min. The widget shows each alert's issue and expiry times and drops alerts past expiry using the clock. Checked reachable from the sandbox on 2026-10-13. Fire-weather (Red Flag Warning, Fire Weather Watch, Fire Warning, Extreme Fire Danger) uses the same API and polling (10 min); the pane shows issue, start and expiry times. With a saved DE station the fire-weather pane makes one extra `alerts/active?point=<lat>,<lon>` request (4 decimals, same 10 min poll, same User-Agent); only the user's own station coordinates are sent, to api.weather.gov. Checked reachable 2026-10-13 and 2026-10-14. With a saved DE station in US waters the marine pane makes one extra `alerts/active?region=<GM|AT|PA|AL|PI|GL>` request (same 10 min poll and User-Agent); the region is chosen on the device from coarse coastline boxes, so only a fixed region code is sent, not coordinates. Checked reachable 2026-10-14. Marine and fire-weather share the URL path `/alerts/active`, so `--fixtures` screenshots need one fixture dir each.
- **Active fires:** NASA FIRMS public active-fire CSV `firms.modaps.eosdis.nasa.gov/data/active_fire/noaa-20-viirs-c2/csv/J1_VIIRS_C2_USA_contiguous_and_Hawaii_24h.csv` (VIIRS 375 m, NRT, rebuilt several times a day). NASA open data, no key (the keyed area API is not used); credit NASA FIRMS (https://www.earthdata.nasa.gov/engage/open-data-services-software-policies). File is about 110 KB, so we poll every 30 min, never faster. Global file is 7 MB and is not used. Checked reachable 2026-10-14. Not yet used: other regions, MODIS, the keyed API.
- **POTA and SOTA spots:** POTA `api.pota.app/spot/activator` and SOTA `api-db2.sota.org.uk/api/spots/50/all/all` (public read APIs, no key; both operated by volunteers). Read-only, one GET each every 2 min with the honest User-Agent, backing off on failure (shared PolledSource). Spots whose comment says QRT are dropped; duplicates per activator+reference collapse to the newest. Credit POTA (pota.app) and SOTA (sota.org.uk) on the card. Reachability from the sandbox not verified; tests use captured fixtures. Terms: https://docs.pota.app/ and https://www.sotadata.org.uk/en/terms.
- **PSKReporter reception reports:** `retrieve.pskreporter.info/query?senderCallsign=<my call>&flowStartSeconds=-900&rronly=1&appcontact=hamclockpro` (public query API, no key). Only for the callsign the user set in Settings, read-only, one GET every 5 min (the rate PSKReporter asks for), backed off on failure (shared PolledSource), honest User-Agent plus `appcontact`. Nothing is requested while the callsign is empty or has characters outside A-Z, 0-9 and one `/`. Credit PSKReporter (pskreporter.info) on the card. Reachable from the sandbox (HTTP 200 on a probe); tests use a captured fixture. Terms: https://pskreporter.info/pskdev.html.
- **HamAlert.org alerts:** the user's own HamAlert telnet feed (`hamalert.org:7300`, DX-cluster `DX de` format), one connection, only when the user saves a HamAlert user name and password in Settings (stored in the owner-only credentials file, password never shown back, logged or put in QML). Receive-only (nothing is sent except login and password), reconnect with the shared backoff. Telnet port is not reachable from the sandbox, so tests use a fake socket and the exact prompts are unverified against the live service. Terms: https://hamalert.org.
- **APRS-IS (nearby stations):** `rotate.aprs2.net:14580`, one connection, receive-only (login `user <call> pass -1 vers hamclock-pro 0.1 filter r/<lat>/<lon>/100`; nothing else is ever sent, so passcode -1 and no beaconing). Only when a callsign is set in Settings; reconnect with the shared backoff; stations older than 30 min dropped, 200 kept. APRS-IS is a free community network with no key; the feed is for non-commercial amateur use. Port 14580 is not reachable from the sandbox, so tests use a fake socket and the live login reply format is unverified. Terms: http://www.aprs-is.net.
- **Launches:** Launch Library 2 (The Space Devs) `ll.thespacedevs.com/2.2.0/launch/upcoming/?limit=10&mode=list`, no key. Free tier is limited to 15 requests per hour per IP and is for non-commercial use; we poll once an hour, cache to disk, and credit "Launch Library 2 (The Space Devs)" on the card status line. Times are NET (no earlier than) and shown "(approx)" when the source gives less than minute precision.
- **hams.at activations:** `hams.at/api/alerts/upcoming` (public read endpoint of the documented hams.at JSON API at `hams.at/api/docs`, no key needed for reading; the optional API key, which only personalises pass predictions, is not used yet and will be C++-only in the credential store). Polled every 10 minutes, cached to disk, credited "hams.at activation alerts" on the card status line; alerts are user-posted, so they are shown as announcements, not verified passes.
- Identify the app honestly in requests (User-Agent with name, version and project URL).

## Status by feature family

From the F01–F30 inventory in Codex's `outputs/hamclock-pro-feature-audit.md`. "Partial" means some code and tests exist, not that it matches 4.22.

| Family | Status | Milestone |
|---|---|---|
| F01 Callsign, station coordinates | Partial: editor, validation, saved settings | M1 |
| F02 Clocks, time zones | Partial: UTC, Big Clock, stopwatch, countdown, manual DE offset | M1 |
| F03 Maidenhead locator | Partial: encode/decode with boundary tests | M1 |
| F04 Sun, Moon | Partial: sunrise/sunset and day length (widget) | WIDGETS D |
| F05 Satellites | Not started | After 1.0 |
| F06 EME | Not started | After 1.0 |
| F07 Propagation, bands | Not started | After 1.0 |
| F08 Space weather | Live: Kp, SFI, X-ray, solar wind, Bz, SSN, trends | WIDGETS B |
| F09 Solar imagery | Done (`solarimage` widget) | 1.0 |
| F10 DE/DX weather | Live: current conditions (Open-Meteo) | WIDGETS E |
| F11 Maps, grayline | Partial: coastlines, land fill, great-circle path; night shade and subsolar marker drawn | M1 |
| F12 Zones, cities, prefixes | Not started | After 1.0 |
| F13 DX cluster | Spot model only | M4 |
| F14 Live spots | Spot model only | After 1.0 |
| F15 ADIF | Not started | After 1.0 |
| F16 Watch lists | Not started | After 1.0 |
| F17 Contests, DXpeditions | Not started | After 1.0 |
| F18 On-the-air | Not started | After 1.0 |
| F19 Beacons | Not started | After 1.0 |
| F20 Radio, rotator | Not started | After 1.0 |
| F21 GPS | Not started | After 1.0 |
| F22 Sensors | Not started | After 1.0 |
| F23 Timers, alarms | Partial: stopwatch and countdown; no alarm sound | M1 (as is) |
| F24 News | Not started | After 1.0 |
| F25 Setup, saved configs | Partial: single station file | M1 / M4 |
| F26 Pane selection | Partial: keyboard pane focus; instrument catalog | M2 |
| F27 Brightness | Not started | After 1.0 |
| F28 Browser, API | Not started | After 1.0 |
| F29 Diagnostics, updates | Not started | M5 (update only) |
| F30 Input, menus, tooltips | Partial: keyboard focus, 44 px targets | M2 / M3 |

## Decisions needed from the owner

These block the milestone shown. Each needs a short written answer in this file or an issue.

1. ~~**M0:** Merge the stacked drafts after a quick review?~~ Yes (Oct 12).
2. ~~**M1:** Drop the SDL2 build and go Qt-only?~~ Yes (Oct 12).
3. ~~**M3:** Which Pi OS image and display?~~ All Pi models, any display including TVs; tiers in "Platform targets" (Oct 12).
4. ~~**M4:** Data sources?~~ NOAA SWPC direct, no default DX cluster node; see "Data source policy" (Oct 12).
5. ~~**M5:** Project name?~~ Keep "HamClock Pro", with an independent-project notice (Oct 12).
6. ~~**M5:** Licence and visibility?~~ Stay MIT; go public at the end of M1 after a sanitizer pass (Oct 12).

No owner decisions are currently open. New questions go in GitHub issues titled "Owner input needed: …".

## Working agreements

- **One writer at a time.** The scheduled Claude agent and interactive sessions work through PRs; the old Codex checkout (`~/Documents/Codex/2026-10-02/…/hamclock-pro`) is retired and must not push.
- **Lighter process per feature:** one short spec section, code, tests, one independent review, and an on-screen check for UI changes. Put findings in the PR description instead of separate audit files. Keep `docs/overnight-checkpoint.md` as a short log, not the place where decisions live.
- **PRs target `main`** and stay small; CI must be green before merge.
- **No simulated data shown as live.** Unavailable panes say so.
- **Hardware claims need hardware.** "Works on Pi X" only after a recorded run on Pi X.

## On the radar (deferred by the owner)

Parked features, not planned now. Revisit if users ask; the issues keep the research (label `deferred`).

| Feature | Why parked | Next step if revived | Issue |
|---|---|---|---|
| Contest calendar pane | WA7BNM Contest Calendar is "All Rights Reserved" with no reuse terms | Ask WA7BNM for permission (draft email in the owner's notes) or find an openly licensed calendar | #200 |
| MUF map style (KC2G) | No documented feed or reuse terms | Ask KC2G for a supported endpoint and attribution | #206 |
| VOACAP-style reliability matrix | Needs a bundled engine (licence review, large) or a hosted API | Evaluate ITURHFProp/VOACAP licences | #206 |
| Tropo forecast map | Owner decision (#281, Oct 15): won't do for now. Needs weather-model data; the usual source has no confirmed reuse terms | GFS/NOMADS refractivity gradient (public domain; needs `nomads.ncep.noaa.gov` allowlisted and a GRIB2 subsetting route light enough for a Pi) | #206, #281 |
| Lightning with range rings | Blitzortung terms are restrictive and it is not allowlisted; NOAA GLM (NCEI NetCDF) is too heavy for a Pi and has no small real sample | Find a light, openly licensed strike feed | #142 |
| NetLogger active nets pane | `api.netlogger.org` fails TLS verification (certificate host mismatch); verification is never disabled | Revisit if NetLogger fixes its certificate; check terms then | #142 |
