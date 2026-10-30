# HamClock Pro 1.0: go/no-go checklist

Target: **submit for review on Monday 2 November 2026.** Prepared Thursday 29 October 2026 by the release-readiness agent against `main` at 441b5b2. Tick items as they are done; re-check anything marked "verify" on the build you actually submit.

| Platform | Ships as | Bundle / package | Status on 29 Oct |
| --- | --- | --- | --- |
| iPhone + iPad | App Store (one universal app, Qt/QML) | `com.chrislaw.hamclockpro` | TestFlight now iPhone + iPad (PR #657 merged). `TESTFLIGHT_UPLOADS` is `false`. |
| Apple TV | App Store, same record (universal purchase), SwiftUI | `com.chrislaw.hamclockpro` | Merged (#659). Never uploaded to TestFlight yet (job skipped while `TESTFLIGHT_UPLOADS=false`). |
| Mac | Notarized Developer ID DMG, outside the App Store | `org.hamclockpro.app` | `apple.yml` signs and notarizes on every main push (notarytool "Accepted" on 29 Oct). Not yet attached to a GitHub release: fixed by PR #681. |
| Raspberry Pi / Linux | `.deb` for Raspberry Pi OS trixie/bookworm, arm64/armhf; SD image | `hamclock-pro` | `release.yml` on every main push and `v*` tags; image on tags. |

Legend: **[OWNER]** only the account holder can do it (App Store Connect, Apple Developer portal, signing, repository settings). Everything else an agent can do in a PR.

## 0. Go/no-go gates (Monday morning)

- [ ] All QA PRs that the owner wants in 1.0 are merged; `main` CI green (CI, Apple, tvOS, Release builds).
- [ ] Full test suite passes on the release commit: `cmake -S . -B build-mac -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" && cmake --build build-mac -j 4 && (cd build-mac && ctest -j 4)`.
- [ ] **[OWNER]** A TestFlight build of iPhone/iPad and of Apple TV from that commit installed and smoke-tested on real devices (section 5).
- [ ] No open issue labelled crash/blocker for any App Store platform (guideline 2.1).
- [ ] Privacy policy and support pages load (both returned HTTP 200 on 29 Oct).
- [ ] Screenshots in `docs/appstore/` still match the UI (re-render if the layout changed).
- [ ] App icon final (logo redesign in draft PR #674). The App Store icon comes from the 1024 px asset in the uploaded build, so the new icon must be merged **before** the build you submit, for iOS (`packaging/apple/ios/Assets.xcassets`), tvOS (layered `App Icon & Top Shelf Image.brandassets` + top shelf) and Mac (`packaging/apple/AppIcon.png`).

**No-go** if: a crash on launch on any device, a pane that shows placeholder or demo data, a link in the app that 404s, or the Apple TV app cannot be left with the Menu button (another agent is fixing this; guideline 4.0/tvOS HIG, a common tvOS rejection).

## 1. Apple Developer portal [OWNER]

Certificates, Identifiers & Profiles > Identifiers.

1. **[OWNER] App ID capabilities for `com.chrislaw.hamclockpro`** (explicit App ID, already exists):
   - **App Groups**: enable and assign `group.com.chrislaw.hamclockpro` (needed by the widgets in PR #655; harmless if #655 isn't in 1.0).
   - **iCloud**: enable, tick **Key-value storage** only (no CloudKit container). Needed for Apple TV settings sync; the entitlement goes into the app in PR #682, which must be merged **only after** this step, because the TestFlight job's profile is created from the App ID's capabilities and signing fails if the entitlement is missing from it.
2. **[OWNER] App Group** (Identifiers > App Groups > +): description `HamClock Pro shared station`, identifier `group.com.chrislaw.hamclockpro`.
3. **[OWNER] Widget extension App ID** (only if PR #655 ships in 1.0): Identifiers > + > App IDs > App, description `HamClock Pro Widgets`, explicit bundle ID `com.chrislaw.hamclockpro.widgets`, capability **App Groups** with `group.com.chrislaw.hamclockpro`.
4. Nothing else: no Push Notifications (pass reminders are local notifications), no Associated Domains, no Sign in with Apple, no multicast networking entitlement (see section 4, WSJT-X multicast).

## 2. App Store Connect app record [OWNER]

My Apps > the existing **HamClock Pro** record (bundle `com.chrislaw.hamclockpro`, created for TestFlight).

### 2.1 Platforms and versions
- [ ] **[OWNER]** The record has the **iOS** platform. Add the **tvOS** platform (Apps > HamClock Pro > sidebar "+" > tvOS); it shares the bundle ID, so it is universal purchase (docs/apple-tv.md section 10).
- [ ] **[OWNER]** Create version **1.0.0** for iOS and for tvOS (version string must equal `CFBundleShortVersionString`, which CI takes from `project(HamClockPro VERSION 1.0.0)` in CMakeLists.txt).
- [ ] **[OWNER]** Attach to each version the TestFlight build you tested (build numbers: iOS = Apple workflow run number, tvOS = tvOS workflow run number).

### 2.2 App Information (shared by iOS and tvOS)
| Field | Enter |
| --- | --- |
| Name | `HamClock Pro` |
| Subtitle | `Ham radio clock and dashboard` |
| Primary category | Utilities |
| Secondary category | Reference |
| Content rights | **Yes**, contains third-party content and you have the rights (public NOAA/NASA/USGS data, public amateur-radio APIs, attribution shown in every pane) |
| Privacy Policy URL | `https://the-chrislaw.github.io/hamclock-pro/privacy/` |
| User Privacy Choices URL | leave empty |
| License agreement | Apple's standard EULA |

Metadata text (description, keywords, promotional text, what's new, support and marketing URLs): `docs/appstore/metadata.md`.

### 2.3 Age rating [OWNER]
Answer every content question **None / No**: no violence, sexual content, profanity, horror, drugs, alcohol or tobacco, gambling, contests, medical or wellness information, unrestricted web access, advertising, messaging/chat, or parental controls; no age assurance. Expected result: **4+** (the lowest rating).
- *User-generated content:* the app displays short DX cluster and spot comments written by other radio amateurs, and it can log in to a cluster node the user chooses. It is not a social network or chat. Recommended answer **No**; if you want to be conservative answer Yes, which may raise the rating and requires a way to report content (guideline 1.2), which the app does not have. Owner's call.

### 2.4 App Privacy (nutrition label) [OWNER]
App Privacy > Get Started:
- "Do you or your third-party partners collect data from this app?" **No, we do not collect data from this app.** Label shows **Data Not Collected**.
- Why that is accurate (from the code, 29 Oct):
  - No analytics, crash reporting, advertising or tracking SDK in any target. No server of our own. Privacy manifests: `NSPrivacyTracking=false`, no tracking domains, no collected data types.
  - Callsign, location and settings are stored on the device (Qt `QSettings`/files; tvOS `UserDefaults` + the user's own iCloud key-value store). Account passwords for optional services live in the Keychain.
  - Data that leaves the device goes directly to the public service the user is using, processed in real time to answer that request: approximate station coordinates to Open-Meteo (weather) and Nominatim (only when **Find** is pressed); the callsign to the DX cluster node, PSKReporter and the Reverse Beacon Network when those features are on; credentials only to QRZ.com / HamQTH / HamAlert / hams.at. Apple's definition of "collect" excludes data the developer and its partners can't access, and data used only in real time to service the request.
  - Device location, camera and motion sensors are **not used** in the shipped build: the sky finder is compiled out (`HAMCLOCK_SKY_FINDER=OFF`). If it is ever turned on, the policy (which says they aren't used) and these answers must change first.
- Third-party services the app contacts (for the policy and for review questions): NOAA SWPC `services.swpc.noaa.gov`, NOAA NHC `www.nhc.noaa.gov`, NWS `api.weather.gov`, USGS `earthquake.usgs.gov`, NASA SDO `sdo.gsfc.nasa.gov`, NASA FIRMS `firms.modaps.eosdis.nasa.gov`, Open-Meteo `api.open-meteo.com`, OpenStreetMap Nominatim `nominatim.openstreetmap.org`, CelesTrak `celestrak.org`, AMSAT `www.amsat.org`, SatNOGS `db.satnogs.org`, Launch Library `ll.thespacedevs.com`, SondeHub `api.v2.sondehub.org`, PSKReporter `retrieve.pskreporter.info`, WSPR `db1.wspr.live` (off by default), POTA `api.pota.app`, SOTA `api-db2.sota.org.uk`, WWFF `spots.wwff.co`, hams.at `hams.at`, HamQSL `www.hamqsl.com`, IOTA `www.iota-world.org`, QRZ.com `xmldata.qrz.com`, HamQTH `www.hamqth.com`, the user's DX cluster node and Reverse Beacon Network telnet, and Windy (`www.windy.com`, a link the user opens, never fetched).

### 2.5 Pricing and availability [OWNER]
- Price: **Free** (Tier 0). All countries/regions.
- **iPhone and iPad Apps on Apple Silicon Macs: untick** (packaging/apple/RELEASE-CHECKLIST.md: the iPad build ignores the hardware keyboard on a Mac; Mac users get the native DMG).
- **Apple Vision Pro: untick** "Make this app available" (iPad apps are on by default; it was never tested there).
- Tax category: leave the default (Software).

### 2.6 Export compliance
Nothing to enter. Every bundle sets `ITSAppUsesNonExemptEncryption = false` (HTTPS through the system and Qt's TLS only; Keychain), so App Store Connect doesn't ask. If it ever asks: "Does your app use encryption?" **Yes** → "Does it qualify for an exemption (Category 5 Part 2 / only standard encryption, e.g. HTTPS)?" **Yes** → no documentation needed.

### 2.7 App Review Information [OWNER]
- **Sign-in required:** No (untick). No demo account: the app has no accounts of its own; optional third-party logins (QRZ.com, HamQTH, HamAlert, hams.at) are not needed to review any feature.
- **Contact:** your first name, last name, phone and email (only Apple sees them). An agent can't fill these.
- **Notes** (paste):

> HamClock Pro is a free, open-source dashboard for licensed amateur ("ham") radio operators. It shows public data from NOAA, NASA, USGS and amateur-radio services; there are no accounts, ads, analytics or in-app purchases.
>
> To try it: on first launch enter any callsign (for example W1AW, a public club station) and a location as a Maidenhead grid square (for example FN31pr) or press Find to look up a place. Each pane's title opens a chooser to pick what it shows; long-press a pane for options; the Map button changes map overlays. On iPhone, swipe the pane left/right.
>
> Panes marked "Set up" need an optional service the reviewer doesn't have to configure: a DX cluster node (a telnet server chosen by the user), or the user's own QRZ.com/HamQTH account for callsign lookups. They are optional and every other pane works without them.
>
> Local network access is requested only if the user turns on a local feature (rig control via rigctld/flrig, gpsd, a Meshtastic node, or the WSJT-X UDP feed).
>
> Apple TV (same purchase): use the Siri Remote. Select on a pane opens the chooser, hold Select or Play/Pause opens pane options, Menu leaves the app.

### 2.8 Screenshots and previews [OWNER uploads]
`docs/appstore/README.md` says which file goes in which slot: iPhone 6.9" and 6.3", iPad 13", Apple TV 1920×1080. No app previews (video) for 1.0.

### 2.9 Submit [OWNER]
- [ ] Version release: **Manually release this version** (so iOS and tvOS can go live together after both are approved).
- [ ] Add for Review (iOS), Add for Review (tvOS), Submit.

## 3. Repository settings [OWNER]
- [ ] Set the variable `TESTFLIGHT_UPLOADS` to `true` when you want the next main push to upload (it was `false` on 29 Oct). Agents don't change it.
- [ ] Secrets already present (names only): `ASC_KEY_ID`, `ASC_ISSUER_ID`, `ASC_KEY_P8_BASE64`, `IOS_DIST_CERT_P12_BASE64`, `IOS_DIST_CERT_PASSWORD`, `MACOS_CERT_P12_BASE64`, `MACOS_CERT_PASSWORD`; variable `APPLE_TEAM_ID`.
- [ ] Tag the release commit: `git tag -a v1.0.0 -m "HamClock Pro 1.0.0" && git push origin v1.0.0`. With PR #681 merged, the tag creates the GitHub release with the Pi `.deb` files, the notarized Mac DMG, the Apple silicon zip and (after pi-gen) the Raspberry Pi SD image.

## 4. Bundle audit (PR #679)

| Item | iOS / iPadOS | tvOS | macOS DMG |
| --- | --- | --- | --- |
| Location `NSLocationWhenInUseUsageDescription` | only when `HAMCLOCK_SKY_FINDER=ON` (off for 1.0); string present in `sky-finder-keys.plist.in` | not used | not used |
| Camera / Motion | same as location (sky finder only) | not used | not used |
| Local network `NSLocalNetworkUsageDescription` | present; now lists rigctld/flrig, gpsd, Meshtastic, DX cluster, WSJT-X | not used | **added** (macOS 15 local network prompt) |
| Notifications | local only (satellite pass reminders); no plist key needed; permission asked when a reminder is first set | not used | n/a |
| `ITSAppUsesNonExemptEncryption` | `false` | `false` | `false` |
| Launch screen | `UILaunchScreen` (system) | not required on tvOS | n/a |
| Orientations / multitasking | iPhone: portrait + both landscapes; iPad: all four, no `UIRequiresFullScreen` (Split View and Stage Manager allowed) | n/a | n/a |
| Icons | single 1024 px `AppIcon` asset (actool makes the rest) | layered 400×240 (@1x, @2x) and 1280×768 App Store icon, top shelf 1920×720 and wide 2320×720 (@1x, @2x) | `AppIcon.icns` from the 1024 px PNG |
| Privacy manifest | UserDefaults CA92.1, FileTimestamp C617.1, DiskSpace **85F4.1** (shown to the user) + E174.1, **SystemBootTime 35F9.1** (added: static Qt's `QElapsedTimer` uses `mach_absolute_time`, and statically linked Qt for iOS brings no manifest of its own). PR #655 adds 1C8F.1 for the App Group. | UserDefaults CA92.1, FileTimestamp C617.1 | not required outside the Mac App Store |
| Version | `CFBundleShortVersionString` = CMake `PROJECT_VERSION`; `--version` and the User-Agent still use a hardcoded `1.0.0` in `main.cpp` until PR #676 merges | from CMakeLists in CI | from CMakeLists |

Regression test: `apple-bundle-metadata-regression` (ctest) checks these against the sources.

Not changed, for the owner to know:
- **WSJT-X multicast on iOS** needs Apple's `com.apple.developer.networking.multicast` entitlement (requested by form, approval takes days). Without it, joining a multicast group fails and the feed reports it; unicast UDP works. Not a 1.0 blocker.
- **Mac DMG and the QML JIT:** the DMG is signed with the hardened runtime and no entitlements. Qt's QML JIT needs `com.apple.security.cs.allow-jit` under the hardened runtime, or it falls back to the interpreter. Notarization passes either way. **Verify**: open the notarized DMG from the GitHub release on a Mac that never had a dev build, launch, and check panes render and scroll smoothly.
- Privacy policy gaps (docs/privacy.md, live page): the provider list leaves out wspr.live (optional WSPR spots), IOTA (iota-world.org group names), AMSAT (backup satellite elements) and a Meshtastic node on the LAN. PR #671 is editing that page; add these there or right after, before submitting. The privacy answers in 2.4 don't change.
- **Mac bundle ID** is `org.hamclockpro.app` (Developer ID needs no App ID). If the Mac App Store PR #643 lands later it uses its own ID; the Keychain service name `org.hamclockpro.app` is shared with iOS by design.

## 5. Device smoke test [OWNER, on the TestFlight builds]

App Review's most common rejections and what to check for each:

| Guideline | Risk here | Check |
| --- | --- | --- |
| 2.1 App completeness (crashes, bugs) | Qt static plugins, first run with no network | Fresh install, airplane mode first launch, then online; rotate; background for 10 min and return; low-power mode |
| 2.3 Accurate metadata | screenshots or description promising panes not in the build | compare `docs/appstore/metadata.md` and screenshots with the build |
| 4.0 Design / tvOS | Menu button must leave the app from the top level | on Apple TV: Menu on the dashboard returns to the Home screen |
| 4.2 Minimum functionality | low for a full dashboard | n/a |
| 5.1.1 Data collection | purpose strings, privacy answers | answers in 2.4; local network prompt appears only after turning on a local feature |
| 2.5.1 / private APIs | none known | upload passes processing without ITMS warnings (check the email after each upload, especially ITMS-91053 "missing API declaration") |
| Broken links | Support/Privacy URLs, in-app links | open both URLs; tap every link the app opens (Windy weather link, SatNOGS satellite pages, GitHub) |
| Placeholder content | "Lorem", demo data, "Set up" panes on default layout | default layout after first run shows live panes only |

Real-device checks the agent couldn't do:
- [ ] **Retina chart rendering:** in the offscreen Retina screenshot runs (device pixel ratio 2 and 3) the **Solar trend** pane drew neither its 30-day flux line nor its Kp bars, and on the phone the **Sun (SDO)** image stayed blank; at ratio 1 both draw. Check both panes on an iPhone and an iPad. If they are blank there too, it's a release bug (Canvas sized by `pixelScale` × device pixel ratio).
- [ ] **iPad portrait:** the station column wraps the DE and DX "Rise in … · Set in …" line (the harness flags it as clipped at 1032×1376).
- [ ] iPad with a hardware keyboard: typing in Settings works (packaging/apple/RELEASE-CHECKLIST.md).
- [ ] Apple TV: first-run setup with the remote, leave running 1 h, Menu exits.

## 6. Raspberry Pi / Linux
- [ ] `.deb` installs on Raspberry Pi OS trixie (arm64) and bookworm (armhf): `sudo apt install ./hamclock-pro_*.deb`, `hamclock-pro --version` prints `1.0.0`.
- [ ] SD image from `image.yml` boots into kiosk mode on a Pi 4 with the 7-inch display (docs/install-pi.md).
- [ ] Release notes say the Pi build is the full app and list known limits (docs/hardware-validation.md).

## 7. CI and release pipeline (reviewed 29 Oct)

| Check | Result |
| --- | --- |
| iOS build number | `github.run_number` of `apple.yml`: increases on every run. A re-run reuses the number and the upload is rejected as a duplicate; push a new commit instead. |
| tvOS build number | `github.run_number` of `tvos.yml`: its own counter. App Store Connect tracks build numbers per platform, so iOS and tvOS numbers may overlap. |
| Marketing version | iOS `MARKETING_VERSION` = `PROJECT_VERSION`; tvOS job reads `project(HamClockPro VERSION …)`; Mac `CFBundleShortVersionString` = `PROJECT_VERSION`; Pi = `1.0.0~git<timestamp>` on main, the tag on `v*`. All 1.0.0. The Pi dev version is now read from CMakeLists too (PR #681). |
| Mac notarization | `apple.yml` macos-universal: Developer ID sign with hardened runtime, `notarytool submit --wait`, `stapler staple` + `validate`. Accepted on 29 Oct. |
| Gap: signed DMG never reached a release | `apple.yml` ran on main only, and the release workflow published an **unsigned** zip. Fixed in PR #681: tags now run `apple.yml`, which attaches `hamclock-pro-macos.dmg` to the tag's release. |
| Gap: two workflows creating the same release | release.yml and apple.yml now both "create, or upload with --clobber if it exists". |
| Gap: SD image on a tag used whatever bookworm `.deb` was newest | `image.yml` grabbed the newest `_bookworm_arm64.deb` from the last 5 releases while the tag's own `.deb` was still building, i.e. the previous dev build. Fixed in PR #681: on a tag it waits for that tag's `.deb`. |
| TestFlight gate | `TESTFLIGHT_UPLOADS == 'true'` on main pushes only; unchanged. |
| GitHub Pages | built from `main:/docs`; two builds "errored" on 29 Oct between successful ones (concurrent pushes). The live pages are fine; re-check the URLs after the last docs merge. |
