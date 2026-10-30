# App Store screenshots: HamClock Pro 1.0

Captured 30 October 2026 (UTC) from the 1.0 code with live public data from that moment (NOAA SWPC, USGS, POTA/SOTA/WWFF, AMSAT TLEs, Open-Meteo). Test station **W1AW, grid FN31pr** (the ARRL headquarters club station), DX **JN58td**, time zone America/New_York. No personal address or private callsign appears.

All files are PNG without alpha, at sizes App Store Connect accepts (checked against Apple's screenshot specifications on 29 October 2026). Up to 10 per device size; order below is the suggested upload order. The app icon is not in any screenshot, so the logo redesign (draft PR #674) does not affect them.

## Where each set goes in App Store Connect

| App Store Connect slot | Folder | Size (px) | Files |
| --- | --- | --- | --- |
| iOS App > iPhone > **6.9" Display** (iPhone with Dynamic Island, large) | `iphone-6.9/` | 1320 × 2868 | `01-dashboard-*`, `02-solar-*`, `03-spots-*` |
| iOS App > iPhone > **6.3" Display** (iPhone with Dynamic Island, medium) | `iphone-6.3/` | 1206 × 2622 | same three |
| iOS App > iPad > **13" Display** | `ipad-13/` | 2752 × 2064 landscape, 2064 × 2752 portrait | `01-dashboard-landscape-*`, `02-dashboard-portrait-*` |
| tvOS App > **Apple TV** | `apple-tv/` | 1920 × 1080 | `01-dashboard-dark`, `02-dashboard-light`, `03-sky-dark` |

Every iPhone and iPad screen comes in `-dark` and `-light`. Suggested order: dark first (01, 02, 03), then the light dashboard, for at most 4–6 per slot. Smaller iPhone and iPad sizes are scaled by App Store Connect from these, so nothing else is needed. Apple's page lists the 6.3" (medium) set as the one iPhone size always required when the 6.9" set is present; uploading both costs nothing.

| File | Shows |
| --- | --- |
| `01-dashboard` | Station strip (UTC and local clocks, DE/DX grids, sunrise/sunset, path), space weather, HF band conditions, gray-line map with the DE–DX path and earthquakes |
| `02-solar` | Sun and Moon times at both ends, solar wind speed and Bz with history |
| `03-spots` | POTA/SOTA/WWFF activators on the air, upcoming amateur satellite passes |
| iPad landscape | Full dashboard: station column, space weather, bands, solar wind, satellites, map |
| iPad portrait | Same in portrait with the Sun image pane |
| TV `01`/`02` | Apple TV dashboard, dark and light |
| TV `03` | Satellites and Sun & Moon panes |

## How they were made (to regenerate)

**iPhone and iPad** (Qt app, offscreen, at the device's logical size and pixel ratio, so the phone layout and the iPad layout are the real ones):

```sh
# station.conf: W1AW 41.7292 -72.7083 (FN31pr), DX 48.1458 11.625 (JN58td), de_zone=America/New_York
build-mac/hamclock-pro-cli --save-station DIR/station.conf W1AW 41.7292 -72.7083 48.1458 11.625 system
echo de_zone=America/New_York >> DIR/station.conf
# options.ini [map]: borders=true; hidden_layers=ota, planets, launches, balloons, balloon-trail,
#   beacons, fires, storms, psk, psk-heard, aprs, adif, mesh, dx-cluster   (a clean map)
QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=3 build-mac/hamclock-pro.app/Contents/MacOS/hamclock-pro \
  --config-dir DIR --fixtures LIVE --theme dark --size 440x956 \
  --pane 1=spaceweather --pane 2=bands --screenshot iphone-6.9/01-dashboard-dark.png
```

- iPhone 6.9": `--size 440x956`, `QT_SCALE_FACTOR=3`. iPhone 6.3": `402x874`, scale 3. iPad 13": `1376x1032` or `1032x1376`, scale 2.
- `LIVE` is a directory of current feed responses saved under each URL's file name (the `--fixtures` transport looks files up by the last path segment), downloaded a minute before rendering so every pane is Live.
- The offscreen grab has no iOS status bar or home indicator; App Store Connect accepts that.

**Apple TV** (SwiftUI app on a dedicated tvOS 27 Simulator, Apple TV 4K at 1080p):

```sh
xcrun simctl launch <udid> com.chrislaw.hamclockpro -hcpResetSettings \
  -hcpStation W1AW FN31pr JN58td America/New_York -hcpPanes spaceweather,bands,solarwind \
  -hcp.display.appearance dark
xcrun simctl io <udid> screenshot --type=png apple-tv/01-dashboard-dark.png   # then drop the alpha channel
```

## Known limits

- Captured from the code on `main` at 441b5b2. If a visible UI change lands before submission (for example the iPad portrait station column, below), re-render.
- iPad portrait: the station column wraps "Rise in … · Set in …" onto two lines (the screenshot harness reports it as clipped). It reads fine but is a layout issue for the adaptive-pane work.
- Panes avoided on purpose because they render without their charts or image in the offscreen Retina (scale 2 and 3) capture: Solar trend (30-day flux line and Kp bars missing) and Sun (SDO) on iPhone (image missing). See the release checklist: these need a check on a real Retina device.
