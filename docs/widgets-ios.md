# iPhone/iPad widgets (WidgetKit), StandBy and CarPlay

Status: first version, built and tested on CI (Simulator); **not yet validated on a device or in a car**.
Backlog item 7 under "More Apple surfaces" in [WIDGETS.md](WIDGETS.md).

## What ships

Three widgets in one WidgetKit extension (`apple/widgets`, bundle id `com.chrislaw.hamclockpro.widgets`),
embedded in the Qt app's `PlugIns/`:

| Widget | Families | Shows |
|---|---|---|
| HF Band Conditions | small, medium, Lock Screen rectangular + inline | small: four band pairs (80-40, 30-20, 17-15, 12-10, best of each pair) for the current period at the station; medium: day and night columns. Same `band_outlook` heuristic as the app (a rough guide, not VOACAP) |
| Space Weather | small, medium, Lock Screen rectangular, circular (Kp gauge), inline | Kp, SFI, GOES X-ray class |
| Next Satellite Pass | small, medium, Lock Screen rectangular + inline | next pass of the first satellite tracked in the app (else the ISS) over the station: rise time or a live countdown in the last hour, max elevation, rise/set direction |

- **StandBy** uses the systemSmall widgets. **CarPlay** (iOS 26+) shows iPhone widgets that support
  systemSmall on the CarPlay dashboard; none of ours opts out (`disfavoredLocations`). A CarPlay *app* is
  not possible: Apple limits CarPlay apps to its approved categories.
- Car/glance design: four values at most, 18-40 pt heavy rounded type, dark card, and every coloured value
  also has a word (Good/Fair/Poor, Quiet/Active/Storm) so meaning survives tinted Lock Screen, StandBy night
  mode and CarPlay rendering.
- Honest states: a Stale/Offline badge and "as of HH:mm" whenever data is not live; last-known values are
  greyed. Without a station the pass widget says "Open HamClock Pro to set your station". Satellite elements
  older than 14 days are flagged.

## How it works

- Data: the same public sources as the app (NOAA SWPC Kp, 10.7 cm flux, GOES X-ray; CelesTrak amateur TLEs),
  fetched in each widget's `TimelineProvider`. Timelines reload about every 30 minutes; the pass timeline also
  has entries at each LOS and one hour before each AOS. TLEs are cached and downloaded at most every 12 hours
  (CelesTrak asks for no more than one download per 2 hours). Cached data is written with
  `completeUntilFirstUserAuthentication`, readable while the phone is locked (CarPlay, StandBy).
- Parsing and astronomy: the Qt-free C++ core shared with the Apple TV app (`apple/tvos/Core`,
  `cmake/TvOSCore`), built for iOS with `cmake/IOSCoreToolchain.cmake`. New in the core for this:
  `hcp_sat_passes`, `hcp_tle_short_name`, `hcp_tle_find_short_name` (tested in `hcpcore_regression`).
- Station: the extension cannot read the app's files, so the Qt app copies callsign, DE position and the
  tracked satellites into the App Group `group.com.chrislaw.hamclockpro` (NSUserDefaults) at start, when the
  station or options change, and when it leaves the foreground (`src/platform/apple_widgetshare*`). The key
  names are a contract tested on both sides (`tests/widget_share_regression.cpp`,
  `apple/widgets/Tests/StationAndFeedTests.swift`). The app cannot call `WidgetCenter.reloadAllTimelines()`
  (Swift-only API); a new station reaches the widgets on their next reload (within ~30 min).

## Build

```sh
for sdk in iphonesimulator iphoneos; do
  cmake -S cmake/TvOSCore -B build-ios-core-$sdk -DCMAKE_OSX_SYSROOT=$sdk \
    -DCMAKE_TOOLCHAIN_FILE=$PWD/cmake/IOSCoreToolchain.cmake && cmake --build build-ios-core-$sdk --target hamclock-core
done
cd apple/widgets && xcodegen generate
xcodebuild test -project HamClockWidgets.xcodeproj -scheme HamClockWidgetsTests -destination 'platform=iOS Simulator,name=iPhone 17 Pro'
```

`WidgetHost` is a Simulator-only stand-in app (same bundle id as the real app, never shipped) for trying the
widgets without a Qt build: install it, add the widgets from the Home Screen gallery, and use its buttons to
share or clear a test station.

## CI (`.github/workflows/apple.yml`)

- `ios-widgets` (every PR): builds the core and the extension for devices and the Simulator, runs the unit
  tests, uploads PNG snapshots of every widget family (`ios-widget-snapshots`, rendered with `ImageRenderer`,
  not the system widget host) and checks that the system registers the extension (`pluginkit`) in a Simulator.
- `ios-simulator` (informational, as before): embeds the extension into the unsigned device app with
  `packaging/apple/embed_widgets.sh` (bundle id prefix, matching versions, arm64) and into the Simulator app.
- `ios-testflight` (main, only while `TESTFLIGHT_UPLOADS` is `true`): builds the extension, and embeds and signs
  it only when the extension's App ID exists and both App Store profiles carry the App Group. Otherwise it
  uploads the app without widgets, exactly as before, with a warning. This path has not run yet.

## Owner: Apple developer portal (once)

No credentials are in the repo or needed from you here; CI keeps using the existing App Store Connect key.

1. Certificates, Identifiers & Profiles > Identifiers > App Groups: register `group.com.chrislaw.hamclockpro`.
2. Identifiers > App IDs: open `com.chrislaw.hamclockpro`, enable **App Groups**, configure it with the group above, save.
3. Register a new App ID `com.chrislaw.hamclockpro.widgets` (explicit), enable **App Groups** with the same group.
4. Nothing else: the TestFlight job creates fresh App Store profiles for both IDs on every run, so they pick up
   the capability. Existing profiles made before step 2 do not need regenerating for CI.
5. CarPlay needs the app on **iPhone**. The TestFlight job currently forces `TARGETED_DEVICE_FAMILY=2` (iPad
   only), so TestFlight builds only offer widgets on iPad (Home Screen, Lock Screen). Shipping on iPhone is a
   separate decision (iPhone screenshots and review); change that line when ready.
