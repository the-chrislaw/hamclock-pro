# App Store metadata draft: HamClock Pro 1.0

Draft text for App Store Connect, prepared 29 October 2026 for the 1.0 submission on 2 November 2026. Paste it into App Store Connect, then edit as you like. Character counts are Apple's limits; every field below fits.

One App Store record with universal purchase (docs/apple-tv.md section 10): bundle ID `com.chrislaw.hamclockpro` for iPhone, iPad and Apple TV. Name, subtitle, category, age rating, privacy answers and URLs are shared by every platform. Description, keywords, promotional text, what's new and screenshots are set per platform: open the **iOS App** version first, then the **tvOS App** version.

The Mac app ships outside the App Store as a notarized Developer ID DMG, so it has no App Store metadata.

## Shared fields (App Information)

| Field | Limit | Value |
| --- | --- | --- |
| Name | 30 | `HamClock Pro` |
| Subtitle | 30 | `Ham radio clock and dashboard` (29) |
| Primary category | | Utilities |
| Secondary category | | Reference |
| Content rights | | "Does not contain, show or access third-party content" is wrong: the app shows public third-party data. Pick **"Yes, it contains third-party content and I have the necessary rights"**. The feeds are public (NOAA, NASA, USGS are US government works; amateur-radio APIs are public with fair-use terms; attribution is shown in each pane). |
| Privacy Policy URL | | `https://the-chrislaw.github.io/hamclock-pro/privacy/` |
| Support URL | | `https://the-chrislaw.github.io/hamclock-pro/support/` |
| Marketing URL (optional) | | `https://github.com/the-chrislaw/hamclock-pro` |
| Copyright | | `2026 Christopher Law` |

Both docs pages answered HTTP 200 on 29 October 2026 (GitHub Pages from `main:/docs`).

## iOS App (iPhone and iPad)

**Promotional text** (170; can be changed any time without review):

> Space weather, HF band conditions, DX and POTA spots, satellite passes and a gray-line world map for your station, at a glance. Free, no account, no tracking.

(158)

**Description** (4000):

> HamClock Pro is a live dashboard for amateur radio operators. Enter your callsign and grid square once, and it shows what you need before you key up: the time at your station and the DX end, space weather, HF band conditions, spots and the sky overhead, on a world map with the day/night gray line.
>
> AT A GLANCE
> • UTC and local clocks for your station and the DX location, sunrise and sunset, short-path and long-path distance and bearing
> • World map with the gray line, your station and the DX, spots, satellites and optional overlays (Maidenhead grid, CQ and ITU zones, aurora, D-region absorption)
> • Pick the panes you want and let each one rotate through a set
>
> SPACE WEATHER AND PROPAGATION
> • Kp index, solar flux, X-ray flares, solar wind and Bz from NOAA SWPC
> • NOAA R/S/G scales and alerts, aurora oval, D-region absorption, Dst
> • HF band outlook for day and night, plus a band-by-hour matrix to your DX
> • Latest solar images from NASA SDO
>
> SPOTS AND ACTIVITY
> • DX cluster spots (choose your node), PSKReporter and the Reverse Beacon Network
> • POTA, SOTA and WWFF activators
> • Watch lists to highlight the calls and entities you're chasing
>
> SKY AND SATELLITES
> • Amateur satellite passes for your location with rise time, maximum elevation and direction
> • Sun and Moon positions, moon phase, EME window, launches and weather balloons
>
> AROUND YOUR STATION
> • Local weather, earthquakes and storm tracking
> • Countdown timers, stopwatch, alarms and a big-clock mode for the shack
>
> PRIVATE BY DESIGN
> HamClock Pro has no accounts, no ads, no analytics and no tracking. Your callsign and location stay on your device. Data comes straight from public sources such as NOAA, NASA, CelesTrak and amateur-radio services; there is no HamClock Pro server in between. Optional services such as QRZ.com or HamQTH use your own account, stored in the Keychain.
>
> Works on iPhone and iPad in portrait and landscape, and on Apple TV with the same purchase. Also available for Mac and Raspberry Pi.
>
> HamClock Pro is an independent, open-source project. It is not affiliated with the original HamClock or its author.

**Keywords** (100, comma-separated, no spaces after commas, don't repeat words from the name):

`amateur radio,ham,HF,propagation,DX cluster,POTA,SOTA,satellite,space weather,solar,Kp,grid,QSO`

(95)

**What's New in This Version** (1.0, 4000):

> First release of HamClock Pro for iPhone and iPad.

**Screenshots:** see `docs/appstore/README.md`.

## tvOS App (Apple TV)

**Promotional text:**

> Turn your TV into a shack display: UTC and local clocks, space weather, HF band conditions, spots and satellite passes on a gray-line world map. No account, no tracking.

(169)

**Description:**

> HamClock Pro puts a live amateur radio dashboard on the big screen. Set your callsign and grid square with the Siri Remote, then leave it running in the shack or the club room.
>
> • UTC and local clocks for your station and the DX location, sunrise and sunset, distance and bearing
> • World map with the day/night gray line, your station and the DX, and satellites
> • Space weather from NOAA SWPC: Kp, solar flux, X-ray flares, solar wind
> • HF band outlook for day and night
> • POTA, SOTA and WWFF activator spots
> • Amateur satellite passes for your location
> • Choose what each pane shows; the screen can dim after hours
>
> HamClock Pro has no accounts, no ads, no analytics and no tracking. Data comes straight from public sources such as NOAA, NASA and CelesTrak.
>
> One purchase covers iPhone, iPad and Apple TV. HamClock Pro is an independent, open-source project, not affiliated with the original HamClock or its author.

**Keywords:** same as iOS.

**What's New** (1.0):

> First release of HamClock Pro for Apple TV.

**Screenshots:** see `docs/appstore/README.md`.

## Checks before pasting

- The tvOS description lists only panes the TV app has (docs/apple-tv.md section 12). If a pane is cut before Monday, delete its line.
- Don't promise features that are off in the shipped build. The sky finder (camera/compass) is off (`HAMCLOCK_SKY_FINDER=OFF`), so it is not mentioned. Widgets are not mentioned because PR #655 is not merged; if it ships in 1.0, add "Home Screen, Lock Screen and StandBy widgets" to the iOS description.
- No prices, "free" claims about future versions, or other platform names in the screenshots (guideline 2.3.10). Mentioning Mac and Raspberry Pi in the description is fine; keep it out of the screenshots.
