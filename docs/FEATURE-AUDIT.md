# Feature parity audit: HamClock Pro vs. the original HamClock

Audited October 13, 2026 by reading the original source (the OpenHamClock fork current users run, `ESPHamClock/`; file:line references are relative to that folder), its `doc/cli/README.md` and `HC_RELEASE-stable.txt`, against `docs/WIDGETS.md`, `docs/ROADMAP.md` and `src/widgets/` on main. Gaps found here were added to the area backlogs in `docs/WIDGETS.md` ("audit additions").

**Totals:** 144 features — 22 covered, 66 partial, 56 missing.

**Key structural finding:** the original receives almost every feed pre-processed from its author's backend server (`wifi.cpp:8 backend_host`; `spacewx.cpp:9-20`; `/worldwx/wx.txt`, `/ONTA/onta.txt`, `/dxpeds/…`, `/contests/…`, `/esats/esats.txt`, `/cty/…`, `/cities2.txt`, VOACAP/MUF/DRAP/cloud map images, lightning/fires/quakes/storms/marine/firewx files). HamClock Pro does not use that server, so map styles and hazard overlays each need a direct, terms-checked source or an offline model (see ROADMAP Data source policy).

## Ten most important gaps

1. **Pane system**: 4 panes + PANE_0 overlay, each with its own rotating set (5/10/30/60 s), forward/back stepping, 42 choices in categories (`HamClock.h:413-456`, `plotmgmnt.cpp:212-258`, `setup.cpp:713-715`).
2. **Map styles and projections**: 12 base styles (Countries, Terrain, DRAP, MUF-VOACAP, MUF-RT, Aurora, Weather, Clouds, Tropo, TOA, REL, User; `HamClock.h:2071-2082`), 4 projections (Mercator, Azimuthal, Azim One, Robinson; `HamClock.h:741-744`), style rotation, auto space-weather map.
3. **VOACAP DE↔DX band conditions** with power, mode, takeoff angle, SP/LP, timeline, antenna models; per-band propagation maps (`wifi.cpp:2020-2170`, `antennas.cpp`).
4. **Live Spots** from PSKReporter, WSPR and RBN (of/by DE, call/grid, distance/count, band stats, map paths; `pskreporter.cpp:662-727`), spot labels and map overlays for all spot sources.
5. **Remote control**: ~55 REST commands (`webserver.cpp:4955-5025`), read-write/read-only live web view (`liveweb.cpp`), mobile dashboard.
6. **Callsign banner and ON AIR** (`callsign.cpp:600-672`), radio tune-from-spot (`radio.cpp:697`).
7. **Clocks**: 7 auxiliary time formats (`HamClock.h:334-340`), time-travel menu (`clocks.cpp:1277-1290`), daily/one-time alarms with GPIO, full Big Clock options (`stopwatch.cpp:1734-1754`).
8. **Space-weather and hazard panes**: DRAP, Aurora, DST, NOAA R/S/G, X-ray history, HamQSL HF/VHF, band-activity heatmap, quakes, storms, marine/fire weather, lightning, fires.
9. **Account-backed panes**: HamAlert, APRS-IS, hams.at, Meshtastic, more On-The-Air programmes, watch lists.
10. **Kiosk/display management**: brightness and light sensor, weekday on/off schedule, idle timeout (`brightness.cpp`), screen lock, restart/exit/reboot/shutdown menu (`ESPHamClock.cpp:2555-2700`), per-category passwords, saved configurations (`configs.cpp:617-639`), demo mode.

## Inventory and comparison

Areas: A UI · B space weather/solar · C spots · D astronomy/satellites · E weather/lookup/events/radio/hardware · F Apple.

### 1. Data panes (PLOT_CH_*, `HamClock.h:413-456`)
| Feature | Evidence | Status | Gap | Area |
|---|---|---|---|---|
| VOACAP_DEDX reliability matrix (power 1 W–1 kW, mode, TOA, SP/LP, timeline) | wifi.cpp:32, 2020-2170 | PARTIAL | B4 is only an SFI/Kp outlook | B |
| DE_Wx / DX_Wx | HamClock.h:414-416, 826-840; wx.cpp | PARTIAL | humidity, pressure+trend, clouds, city, DX tz | E |
| DX_Cluster | dxcluster.cpp:1-5, 787-793 | PARTIAL | map overlay, watch list, commands, UDP, tap-to-DX/tune | C |
| Solar_Flux, Planetary_K, Sunspot_N, Solar_Wind, Bz_Bt | spacewx.cpp:9-14 | COVERED | (flux history 30 vs 99 days) | B |
| X-Ray history | spacewx.cpp:15 | PARTIAL | current class only | B |
| DRAP, Aurora, DST | spacewx.cpp:13, 18, 19 | MISSING | — | B |
| NOAA_SpcWx R/S/G | spacewx.cpp:17 | PARTIAL | B6 text only | B |
| SDO images (Composite, Magnetogram, 6173, 131, 193, 211, 304; rotate; movie) | sdo.cpp:20-37, 317-323 | COVERED/PARTIAL | missing 131/211/composite, movie | B |
| Moon (image, phase, EME/movie links) | moonpane.cpp:97-98 | PARTIAL | no image, no EME link | D |
| Rotator (rotctld, track sat/DX) | gimbal.cpp:1-40, 1031, 1157 | PARTIAL | "rotator later" | E |
| ENV temp/press/humid/dew (2× BME280) | HamClock.h:424-427; BME280.cpp | PARTIAL | sensors planned | E |
| Countdown as a pane | HamClock.h:431; stopwatch.cpp:2471 | PARTIAL | dialog only | A |
| Contests | contests.cpp:20-22 | MISSING (no widget; contest calendar parked, ROADMAP #200; re-checked Oct 29) | E3 | E |
| Live_Spots PSK/WSPR/RBN | pskreporter.cpp:662-727 | PARTIAL | PSKReporter only | C |
| On_The_Air (POTA, SOTA, WWFF, IOTA, GMA, LLOTA, WWTOTA, TOWERS, WWBOTA) | ontheair.cpp:66-76, 191-290 | PARTIAL | POTA/SOTA only | C |
| ADIF log pane (watch, sort, map, set_adif) | adif.cpp; webserver.cpp:4980 | PARTIAL | roadmap only | C |
| DXPeditions (hide list, current only, Set DX) | dxpeds.cpp:363, 476 | MISSING (no widget; re-checked Oct 29) | hide/Set DX unmentioned | E |
| Storms, Nets, Marine_Wx, Fire_Wx, Quakes | hurricane.cpp, activenets.cpp, marinewarnings.cpp, firewx.cpp, quakes.cpp | MISSING | — | E |
| Launches, Eclipse, Balloons | launches.cpp, eclipsepane.cpp, balloons.cpp | MISSING | — | D |
| HF_Bands / VHF_Cond (HamQSL) | hamqsl.cpp:1-6 | PARTIAL / MISSING | own model instead of feed | B |
| Sat_Alerts (hams.at) | hamsat.cpp:1-8 | PARTIAL | settings row only | D |
| Mesh_Mon, Nearby_APRS, HamAlert | meshtastic.cpp, aprscluster.cpp, hamalert.cpp | PARTIAL | settings rows only | C |
| Band_Act heatmap | bandactivity.cpp | MISSING | — | C |

### 2. Pane system
| Feature | Evidence | Status | Area |
|---|---|---|---|
| 4 panes + PANE_0 overlay | HamClock.h:471-477, 2573-2579 | PARTIAL | A |
| Per-pane rotation 5/10/30/60 s; forward/back | plotmgmnt.cpp:1-3; setup.cpp:713-715 | PARTIAL/MISSING | A |
| NCDXF status box rotation (beacons, display, sensors, wx, lightning) | HamClock.h:712-721 | PARTIAL | A |
| Space-wx stats ranked by impact | spacewx.cpp:46, 175 | MISSING | B |
| User BMP as pane/map (set_bmp) | webserver.cpp:4984; mapmanage.cpp:479 | MISSING | A |

### 3. Map
| Feature | Evidence | Status | Area |
|---|---|---|---|
| Projections Mercator/Azimuthal/Azim One/Robinson; centre longitude | HamClock.h:741-744; setup.cpp:414 | PARTIAL | A |
| Tap popup Set DE/DX, zoom 1–4×, recenter | earthmap.cpp:468-640 | PARTIAL | A |
| Styles: Countries/Terrain, DRAP, MUF-VOACAP, MUF-RT, Aurora, Weather, Clouds, Tropo, TOA/REL | HamClock.h:2071-2082 | MOSTLY MISSING | A/B/E |
| Style rotation, Auto SpcWx map, legends | setup.cpp:717-719; mapmanage.cpp | MISSING | A |
| Night shading | earthmap.cpp:1025 | COVERED | A |
| Grids (tropics, lat/lng, Maidenhead, azimuth, CQ, ITU), cities, borders/states | HamClock.h:759-769; zones.cpp; cities.cpp; borders.cpp | PARTIAL/MISSING | A |
| Lightning rings, fires badge | lightning.cpp; fires.cpp | MISSING | E |
| RSS banner | earthmap.cpp:1024; rss.cpp | PARTIAL | E |
| DE antipode, long-path drawing, sublunar | earthmap.cpp:22-34, 180-230 | PARTIAL | A |
| Spots/overlays for cluster, PSK, ONTA, ADIF, DXpeds, HamAlert, hams.at, nets, launches, balloons, mesh, hazards | drawXOnMap in each file | PARTIAL/MISSING | C/D/E |
| Spot labels, min label distance, disable spots | HamClock.h:3107-3110; setup.cpp:923-934 | MISSING | C |
| Planets | planets.cpp | MISSING | D |
| Satellite path/footprint; NCDXF beacons | earthsat.cpp; ncdxf.cpp | COVERED | D |
| Hover info box | infobox.cpp:17-669 | MISSING | A |
| Gray display; ADS-B/Windy/WEFAX badges; MOTD/QR | setup.cpp:943-945; adsbbadge.cpp; windbadge.cpp; wefax.cpp; motd.cpp | MISSING | A/E |

### 4. DE/DX panels
| Feature | Evidence | Status | Area |
|---|---|---|---|
| DE formats (All info, analog, calendar, annotated analog, digital 12/24) | HamClock.h:1332-1337 | PARTIAL | A |
| DE/DX time-zone menu | clocks.cpp:1444 | PARTIAL (no DX zone) | A |
| Sunrise/set in/ago vs at | HamClock.h:703-708 | PARTIAL | D |
| SP/LP toggle, magnetic bearing (WMM), units | earthmap.cpp:1793-1880; magdecl.cpp | PARTIAL | A |
| New DE/DX by lat/lng or grid; prefix → country | asknewpos.cpp; prefixes.cpp | COVERED | A/C |
| Weather popup on new DE/DX | setup.cpp | MISSING | E |
| Callsign banner + ON AIR, colours, rainbow; public IP | callsign.cpp:600-672 | MISSING | A |

### 5. Clocks and time
| Feature | Evidence | Status | Area |
|---|---|---|---|
| UTC clock tap toggles UTC/DE | clocks.cpp:1264-1266 | COVERED | A |
| Aux time (Date, DOY, JD, MJD, Sidereal, Solar, UNIX) | HamClock.h:334-340 | MISSING | A |
| Change time / time travel | clocks.cpp:1277-1290 | MISSING | A |
| Big Clock full options | stopwatch.cpp:1734-1754 | PARTIAL | A |
| Stopwatch, countdown | stopwatch.cpp | COVERED | A |
| Daily/once alarms (GPIO, REST) | stopwatch.cpp:120-131, 660 | PARTIAL | A |
| Date order, week start | setup.cpp:657-661 | MISSING | A |
| NTP choice | setup.cpp:375, 774-777 | PARTIAL | E |

### 6. Spot plumbing
| Feature | Evidence | Status | Area |
|---|---|---|---|
| Cluster SSID login + 12 native commands | setup.cpp:383-395, 625-636 | PARTIAL | C |
| UDP spots (WSJT-X, N1MM/DXLog, Log4OM), multicast | parsespot.cpp:103-408 | PARTIAL | C |
| Watch lists Off/Red/Only/Not; not-in-log keywords | HamClock.h:3226-3251; watchlist.cpp | PARTIAL | C |
| Cluster filters | dxcluster.cpp:787-793 | COVERED | C |
| Bio lookup (opens web page) | HamClock.h:2714-2718 | PARTIAL (we do XML lookup) | E |
| IOTA names, xOTA markers | iota.cpp; xota.cpp | MISSING | C |
| Tap spot → set DX + tune | dxcluster.cpp:301 | PARTIAL | C/E |

### 7. Satellites and astronomy tools
| Feature | Evidence | Status | Area |
|---|---|---|---|
| Up to 2 satellites; path | earthsat.cpp | COVERED | D |
| Sky plot, co-visibility planner, group schedule, AMSAT status/links | earthsat.cpp; sattool.cpp; satsked.cpp | PARTIAL/MISSING | D |
| Sat rise/set alarm; user TLE; max TLE age | ArduinoLib.h:229; webserver.cpp:5006 | PARTIAL (rise alerts for any number of selected satellites: Apple notifications, in-app banner at lead time and AOS, Linux chime; no LOS alert, no GPIO sat-alarm pin, no user TLE; TLE age fixed at 30 days) | D |
| EME tool | emetool.cpp | COVERED | D |
| Yearly grayline chart | grayline.cpp | MISSING | D |

### 8. Settings and system
| Feature | Evidence | Status | Area |
|---|---|---|---|
| Call, lat/lng (DMS), grid | setup.cpp:369-372 | PARTIAL | A |
| gpsd/NMEA (time + location), IP geolocation | setup.cpp:373-374, 607-613 | PARTIAL/MISSING | E |
| Wi-Fi | setup.cpp:376-377 | MISSING (the Pi image does not configure Wi-Fi; set it in Raspberry Pi Imager; re-checked Oct 29) | A |
| Units, list scroll direction | setup.cpp:664-676 | MISSING | A |
| Colour editor | setup.cpp:1134-1300 | PARTIAL | A |
| Brightness, light sensor, on/off schedule, idle | brightness.cpp | PARTIAL | A |
| Touch calibration | touch.cpp:93 | PARTIAL | A |
| Saved configurations | configs.cpp:617-639 | PARTIAL | A |
| Per-category passwords | passwd.cpp | PARTIAL | A |
| Lock/Demo/Restart/Exit/Reboot/Shutdown menu; demo mode | ESPHamClock.cpp:2555-2700; webserver.cpp:57-83 | MISSING | A |
| Auto/manual upgrade | setup.cpp:1005-1012; OTAupdate.cpp | PARTIAL | A |
| On-screen keyboard with paste | askmodal.cpp | PARTIAL | A |
| CLI options | doc/cli/README.md | PARTIAL | A |

### 9. Remote control
| Feature | Evidence | Status | Area |
|---|---|---|---|
| REST get_* (config, de, dx, spots, spacewx, sys, time, voacap …) | webserver.cpp:4957-4979 | PARTIAL (after 1.0) | A |
| REST set_* (alarm, cluster, mapview, newde/newdx, pane, satname, screenlock, time, voacap …) | webserver.cpp:4980-5024 | PARTIAL | A |
| Remote screenshot, live web view 8081/8082, mobile dashboard | webserver.cpp:4958-4960; liveweb.cpp | PARTIAL/MISSING | A |
| Antenna models for VOACAP | antennas.cpp | MISSING | B |

### 10. Hardware
| Feature | Evidence | Status | Area |
|---|---|---|---|
| GPIO (MCP23017): countdown LEDs/button, alarms, sat alarm, ON AIR, user pins | ArduinoLib.h:224-242 | MISSING | E |
| Radio (rigctld/flrig/KX3): PTT → ON AIR; set frequency | radio.cpp:697 | PARTIAL | E |
| Rotator auto-track | gimbal.cpp | PARTIAL | E |
| PiAware host | setup.cpp:404 | PARTIAL | E |
| CPU temp, Wi-Fi RSSI, disk, IP status | cputemp.cpp; wifimeter.cpp | PARTIAL | E |

## Additions in HamClock Pro not in the original
QRZ/HamQTH XML lookup (original only opens a web page); direct public data sources (NOAA SWPC, Open-Meteo, CelesTrak + SGP4) instead of the author's backend; NOAA alerts text; TV mode and 800×480–4K scaling; light/dark themes; macOS and iPad apps with Keychain; data-source attribution screen; SD-card image with first-run setup. Ambiguous: the original's MUF-RT source is hidden behind its backend (KC2G is a candidate).
