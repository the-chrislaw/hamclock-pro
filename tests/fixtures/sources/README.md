# Real source samples (captured 2026-10-13 ~17:40 UTC)

The cloud agents' sandbox cannot reach outside websites, so these one-time samples were captured from a normal connection with the User-Agent `HamClockPro/0.1 (+https://github.com/the-chrislaw/hamclock-pro)`. Copy the parts a widget needs into `src/widgets/<id>/tests/fixtures/`, named after the URL's file name, and trim further. Do not refresh these from CI.

| File | URL | Notes |
|---|---|---|
| swpc-rtsw-wind-1m.json | https://services.swpc.noaa.gov/json/rtsw/rtsw_wind_1m.json | Real-time solar wind (speed, density, temperature). **Newest first.** First 60 rows. The older `products/solar-wind/plasma-*.json` URLs now return 404. |
| swpc-rtsw-mag-1m.json | https://services.swpc.noaa.gov/json/rtsw/rtsw_mag_1m.json | Real-time IMF Bt/Bz. Newest first. First 60 rows. `products/solar-wind/mag-*.json` returns 404. |
| swpc-observed-solar-cycle-indices.json | https://services.swpc.noaa.gov/json/solar-cycle/observed-solar-cycle-indices.json | Monthly sunspot number and F10.7. Last 36 months. |
| swpc-planetary-k-index-forecast.json | https://services.swpc.noaa.gov/products/noaa-planetary-k-index-forecast.json | 3-day Kp observed + predicted. |
| swpc-alerts.json | https://services.swpc.noaa.gov/products/alerts.json | Alerts/watches/warnings. First 12. |
| swpc-f107-cm-flux.json | https://services.swpc.noaa.gov/json/f107_cm_flux.json | Daily 10.7 cm flux history (trend chart). |
| sdo-latest-512-0193.jpg | https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_0193.jpg | SDO AIA 193 Å latest image, 512 px. Other channels: `latest_512_0304.jpg`, `latest_512_HMIIC.jpg`, etc. NASA imagery, public domain with credit. |
| pota-spot-activator.json | https://api.pota.app/spot/activator | POTA activator spots. First 25. |
| sota-spots.json | https://api-db2.sota.org.uk/api/spots/50/all/all | SOTA spots. First 25. The older `api2.sota.org.uk/api/spots/50/all` returns a deprecation notice instead of spots. |
| pskreporter-query.xml | https://retrieve.pskreporter.info/query?senderCallsign=W1AW&flowStartSeconds=-900&rronly=1&appcontact=hamclockpro | Reception reports (XML). PSKReporter asks clients to query at most every 5 minutes and to include `appcontact`. |
| celestrak-amateur.tle | https://celestrak.org/NORAD/elements/gp.php?GROUP=amateur&FORMAT=tle | Amateur satellite TLEs. CelesTrak asks for at most one download per group per 2 hours; cache daily. |
| open-meteo-current.json | https://api.open-meteo.com/v1/forecast?latitude=37.77&longitude=-122.42&current=temperature_2m,relative_humidity_2m,wind_speed_10m,wind_direction_10m,weather_code,is_day&timezone=UTC | Current weather (no key; free for non-commercial use, attribution required). |
| nominatim-search.json | https://nominatim.openstreetmap.org/search?q=Pioneer+Courthouse+Square,+Portland,+Oregon&format=jsonv2&limit=3&addressdetails=1 | OpenStreetMap geocoding for the Settings location lookup. Usage policy: max 1 request/second, identify the app in User-Agent, only on explicit user action, attribution "© OpenStreetMap contributors". Captured 2026-10-13. |

Not included: the NG3K DXpedition list and the WA7BNM contest calendar are copyrighted listings whose terms must be checked (and recorded in the roadmap's Data source policy) before any use; ask the owner via an "Owner input needed" issue if unclear.
