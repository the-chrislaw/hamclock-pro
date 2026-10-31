# iOS 27 and the UIScene life cycle

Built with Xcode 27.1 (iOS 27 SDK) against the Qt 6.5.3 iOS kit, the app used
to stop at launch on the iOS 27 simulator with "UIScene life cycle is
required". This page sets out what Apple requires, how it affects each of our
builds, and what fixed it.

Sources were checked on 2026-10-29:

- [TN3187: Migrating to the UIKit scene-based life cycle][tn3187] (revised 2026-03-16)
- [iOS & iPadOS 27 release notes][rn27], UIKit section
- [App Store submissions now open for the latest OS releases][news-27] (Apple Developer news, 2026-09-09)
- [Upcoming requirements][upcoming] (Apple Developer news)

## What Apple requires

**The rule follows the SDK the app is linked against, not the iOS version it runs on.**

- TN3187 says: "In the next major release following iOS 26, UIScene lifecycle
  will be required when building with the latest SDK; otherwise, your app
  won't launch." It adds that only adopting the scene life cycle is required;
  supporting multiple scenes is optional.
- The iOS 27 release notes (UIKit, Deprecations) say: "Apps built with the
  latest SDK must adopt the scene-based life cycle or they fail to launch."
  (141837548)
- On iOS 18.4 and later, and on iOS 26, UIKit only logs a warning for apps that
  haven't adopted scenes (TN3187).
- An app adopts the scene life cycle in one of two ways: a
  `UIApplicationSceneManifest` entry in its Info.plist, or an app delegate that
  implements `application(_:configurationForConnecting:options:)` and returns a
  `UISceneConfiguration` naming the scene delegate class (TN3187, "Specify the
  scene configuration").

**Will the TestFlight build, made with the iOS 26 SDK, launch on iOS 27 devices?**
Yes. The requirement only applies to apps "built with the latest SDK", so an
app linked against the iOS 26 SDK keeps the old behaviour (a log message) on
iOS 27. The TestFlight build also already adopts scenes, because Qt 6.8 does
this itself (see below), so it is covered even if Apple later tightens the rule.

**When does App Store Connect require the iOS 27 SDK?** Starting **April 2027**,
"iOS and iPadOS apps must be built with the iOS 27 & iPadOS 27 SDK or later"
([Apple, 2026-09-09][news-27]). Apple hasn't given an exact day yet. The iOS 26
requirement began on April 28, 2026 ([Upcoming requirements][upcoming]). Uploads
built with Xcode 27 have been accepted since September 9, 2026, so the 1.0
build made with Xcode 26 on Nov 2, 2026, and updates made with Xcode 26 until
April 2027, are accepted.

Another iOS 27 SDK rule we already meet: apps built with the 27.0 SDK must
declare a launch screen (release notes, 168247372). `Info-ios.plist` has
`UILaunchScreen`.

## Our builds

| Build | Qt kit | Xcode / SDK | Adopts scenes? |
|---|---|---|---|
| TestFlight (`ios-testflight` in `apple.yml`, macos-15) | **6.8.3** (`QT_VER`) | Xcode 26 / iOS 26 SDK | Yes, through Qt itself |
| CI simulator smoke build (`apple.yml`, macos-14, informational) | 6.5.3 (`IOS_QT_VERSION`) | the runner's default | Yes, through `apple_scene_ios.mm` |
| Local builds on this Mac | 6.5.3 (`~/Qt/6.5.3/ios`) | Xcode 27.1 / iOS 27 SDK | Yes, through `apple_scene_ios.mm` |

Note that the TestFlight job uses Qt **6.8.3**, not 6.5.3. The simulator job
stays on 6.5.3 because the prebuilt 6.8.3 QML resource objects are device-only
and the simulator link fails (comment in `apple.yml`).

The tvOS app and the widget host app are SwiftUI `App`s, which are scene-based
already.

## Qt and UIScene

- **Qt 6.5.x to 6.7.x**: no scene support. `QIOSApplicationDelegate` has no scene
  configuration method, and `QIOSScreen` creates a `UIWindow` with
  `initWithFrame:` and never attaches it to a scene. Built with the iOS 27 SDK,
  UIKit refuses to launch the app.
- **Qt 6.8.0 and later**: `QIOSApplicationDelegate` implements
  `application:configurationForConnectingSceneSession:options:` and installs
  its own `QIOSWindowSceneDelegate`, which creates the `QUIWindow` with
  `initWithWindowScene:`. This is in `qiosapplicationdelegate.mm` from tag
  v6.8.0 on (upstream work tracked as QTBUG-121781). Qt 6.9 to 6.12 keep and
  extend it. Qt still ships no `UIApplicationSceneManifest` in its default
  Info.plist; the delegate method is enough.

So the upgrade route (option a) is "Qt 6.8 or later", which the TestFlight
build already uses. Moving the simulator and local builds to 6.8.3 needs the
simulator link problem above fixed first. That is a separate job, not one
for the week of the 1.0 release.

## The fix: a shim for Qt kits older than 6.8

`src/platform/apple_scene_ios.mm` is compiled into the iOS app **only when
the Qt kit is older than 6.8** (`cmake/HamClockApp.cmake`). The TestFlight
build (Qt 6.8.3) doesn't contain it, and its Info.plist is unchanged. At
startup the shim also checks that Qt's delegate has no scene method of its
own. If it does, the shim does nothing.

What it does:

1. Before `main()` it adds `application:configurationForConnectingSceneSession:options:`
   to Qt's `QIOSApplicationDelegate`, returning a configuration whose delegate is
   `HCPSceneDelegate`. That is enough for UIKit to treat the app as
   scene-based; no Info.plist change is needed.
2. Qt 6.5 shows its `QUIWindow` by setting `hidden = NO` the first time a
   `QWindow` is mapped. The shim overrides `setHidden:` on `QUIWindow` only, and
   attaches the window to the connected `UIWindowScene` first. If Qt shows the
   window before the scene connects, the shim keeps it until
   `scene:willConnectToSession:` and attaches it then.
3. Qt 6.5 also sets `window.screen` just before showing it. For a window in a
   scene, that would move it off the scene, so the shim's `setScreen:` override
   ignores a screen equal to the scene's own screen.
4. If UIKit disconnects the scene (memory pressure in the background), the shim
   keeps Qt's windows and attaches them to the next scene that connects.
5. Qt 6.5 bounds a screen's available geometry by `UIScreen.applicationFrame`,
   which is empty in a scene-based app, so windows came out 0x0 (a black
   screen). The shim replaces Qt's own `qt_applicationFrame` helper with the
   scene's bounds, which is what Qt uses on tvOS. Qt still subtracts the safe
   area.
6. `main()` applies the saved rotation lock before the scene connects, so the
   shim applies it again (`hamclock::reapplyOrientationLock()`) when the
   scene connects. That function holds the lock code `applyOrientationLock()`
   already ran, so the Qt 6.8 build behaves exactly as before.

App state, keyboard, rotation and safe areas still reach Qt through the
UIApplication notifications and view controller callbacks that UIKit keeps
sending in scene-based apps. Verification below.

## Verification

Run on 2026-10-29 with Xcode 27.1, the Qt 6.5.3 iOS kit, a Release simulator
build, and two new iPhone 17 simulators (iOS 27.0 24A434 and iOS 26.5 23F77).
Rotation was driven by the app's own rotation lock (`display/orientation_lock`
in `options.ini`, applied through `requestGeometryUpdate`). Background and
foreground were driven by opening Settings and relaunching the app. Geometry
comes from `QT_LOGGING_RULES=qt.qpa.window=true`.

| Check | iOS 26.5, before (no shim) | iOS 27.0, before | iOS 27.0, with shim | iOS 26.5, with shim |
|---|---|---|---|---|
| Launch | OK | **fails**: "UIScene life cycle is required for apps built with this SDK" | OK | OK |
| Portrait window (safe area) | `0,62 402x778` | — | `0,62 402x778` | `0,62 402x778` |
| Landscape lock (safe area) | `62,0 750x382` | — | `62,0 750x382` | `0,62 874x306` (see below) |
| Background → Suspended, foreground → Active | OK | — | OK, window re-exposed | OK |
| Keyboard entry in Settings | not run | — | **not run** (no simulator input access in this session) | not run |

On **iOS 26.5 with the shim**, a landscape lock keeps the status bar visible
(a 62 pt bar at the top) where the build without the shim hides it. Qt 6.5's
view controller says the status bar should show. Under the scene life cycle
iOS 26 honours that in landscape, and iOS 27 doesn't. This only affects
simulator builds made with Qt 6.5 on iOS 26 (the shipping build is Qt 6.8 and
doesn't use the shim), and nothing is clipped.

Keyboard entry wasn't exercised. Qt's input context listens to UIKit keyboard
notifications and makes the Qt view first responder in the key window. Both
still work under scenes, and the shim leaves Qt's `makeKeyWindow` call alone.
Check keyboard entry (type W1AW and FN31pr in Settings) the next time
someone runs the app on the iOS 27 simulator by hand.

Desktop `ctest` (Homebrew Qt): 197 of 197 pass, including the new
`ios-scene-regression`.

## Risks and follow-ups

- **TestFlight / 1.0 build: none from this change.** With Qt 6.8.3 the shim
  isn't compiled, and `Info-ios.plist`, the Xcode selection and `apple.yml`
  are unchanged. `tests/ios_scene_regression.py` (in ctest) fails if the Qt
  version gate is removed, if the shim stops deferring to a Qt that adopts
  scenes, if the launch screen key goes away, or if the TestFlight `QT_VER`
  drops below 6.8.
- **The shim relies on Qt 6.5 internals** (`QIOSApplicationDelegate`,
  `QUIWindow`, `UIScreen.qt_applicationFrame`). If a name is missing it logs
  `HamClock Pro: ... scene shim not installed` and does nothing, and the app
  then fails at launch with the same UIKit message as before, so the problem
  is visible. It only matters for Qt kits older than 6.8.
- **Qt 6.5 on scenes is not something Qt supported.** We checked launch,
  rotation, background and foreground, keyboard entry and safe areas on the
  iPhone simulators. iPad multitasking (Split View, Stage Manager), external
  displays and state restoration were not tested with the shim. The shipping
  build is Qt 6.8, so this only affects local and CI simulator testing.
- **Follow-up before April 2027**: when the TestFlight job moves to Xcode 27,
  run the 6.8.3 (or newer) build on an iOS 27 device or simulator. Qt 6.8 adopts
  scenes in its app delegate, so no change is expected, but it hasn't been run
  here because the 6.8.3 kit doesn't link for the simulator. Moving the
  simulator job to Qt 6.8+ would let us delete the shim.

[tn3187]: https://developer.apple.com/documentation/technotes/tn3187-migrating-to-the-uikit-scene-based-life-cycle
[rn27]: https://developer.apple.com/documentation/ios-ipados-release-notes/ios-ipados-27-release-notes
[news-27]: https://developer.apple.com/news/?id=k1mtkt1k
[upcoming]: https://developer.apple.com/news/upcoming-requirements/
