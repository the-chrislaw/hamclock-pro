# Settings and first run: making HamClock Pro simple

Research and proposal, Oct 29 2026, for 1.0 (Mon Nov 2). Docs only: nothing in the app changes until the owner picks items from the lists at the end.

**Short version.** Today's Settings is one long page of 20 cards and about 80 settings, with 8 separate Save buttons plus Done. On the Pi it is 9 screens long. Most people need 3 things: callsign, location and maybe the units. The proposal:

- A 3-step first run: callsign, location, check and go.
- A Settings home that fits on one screen and opens sections one level deep.
- Expert options stay, but sit one tap further away.
- Spot filters and map options also open from the pane or map they change.

For Monday, ten small, low-risk changes get most of the benefit. They're listed at the end.

---

## 1. Today's Settings, measured

Rendered from `main` at c1c9c62 with the offscreen harness. Test station W1AW at 41.7292, -72.7083, DX Tokyo, dark theme. The renders scrolled page by page, using a scratch-only patch that was not committed.

| Size | Visible page | Total length | Screens to scroll |
|---|---|---|---|
| Phone 390×844 | 794 px | 4586 px | **5.8** |
| Pi 1024×600 | 542 px | 4899 px | **9.0** |
| Mac 1512×982 | 856 px | 5603 px | **6.5** |

The phone render uses the Mac build, so it also shows the cards that iOS hides: Exit, TV overscan, Screen blanking and Station hardware. A real iPhone is about a screen shorter.

Screenshots:
- [current-phone-all.png](mockups/current-phone-all.png): every phone screen side by side.
- [current-pi-station.png](mockups/current-pi-station.png)
- [current-pi-cluster.png](mockups/current-pi-cluster.png)

### Counts (Mac/Pi build)

| | Today |
|---|---|
| Cards on one page | 20 (21 on iOS counting Screen rotation; Map size is hidden on phones) |
| Stored settings | ~80 |
| Settings in the "DX cluster" card alone | 40 |
| Save buttons | 8, plus Done |
| Ways a change gets saved | 3: instantly (chips), on Save, or on Done |
| Text in the "Data sources" card | 17 paragraphs, mid-page, before Exit and DX cluster |

### Inventory

How to reach each setting:
- **Pi:** tap ⚙ on the station card, then scroll.
- **Mac:** Settings button or ⌘-comma, then scroll.
- **iPhone:** Settings button, then scroll.

"Taps" means taps after Settings is open; "scroll" is in Pi screens. "Who" is E for everyone, S for some users, X for experts.

| Card (order on page) | Settings | Who | Scroll (Pi) | If left at default |
|---|---|---|---|---|
| Station | callsign, place search, grid, home lat, home lon, DX lat, DX lon, time zone, Suggest zone, DX zone + Auto, Save | E (DX: S) | 0–1 | Nothing works until callsign and location are set. DX defaults to the saved value. The time zone defaults to **the device's zone, not the station's** (bug B2). |
| Appearance | System / Light / Dark | E | 1 | Follows the system. Fine. |
| Distance units | km / miles | E | 1 | **Always km**, even for US users (S3). |
| Clock format | date order, 12/24 h, week start | S | 1 | Always "Thu 15 Oct", 24 h, Monday. |
| Callsign banner | 6 colours, 5 backgrounds, custom title + its own Save | S | 1–2 | Fine. **Title typed and then Done is lost** (B3). |
| Saved configurations | name, Save, Rename, pick, Restore, Delete | X | 2 | Fine. |
| Text size | 100/115/130/150 % | S | 2 | Fine. |
| Screen rotation (iOS) | Auto / Portrait / Landscape | S | — | Fine. |
| Map size (not phone) | Large / Medium / Small | S | 2–3 | Fine. |
| Accent colour | 5 colours | S | 3 | Fine. |
| Big Clock face | Digital / Analog | S | 3 | Fine. |
| TV overscan (not iOS) | 0–10 % | X (TV only) | 3 | Fine. |
| Screen blanking (not iOS) | idle minutes, night window, 7 weekdays | S (kiosk) | 3–4 | Never blanks. Fine. |
| Demo mode | Start/Stop | X | 4 | Off. |
| Data sources | text only | — | 4–5 | — |
| Exit (not iOS) | Exit, tap again | S (Pi) | 5 | — |
| DX cluster | host, port, SSID, login commands, UDP port, UDP LAN, multicast group, UDP by-me, UDP sets DX, APRS host/port/radius, Mesh host/port, PSK radius, WSPR, RBN, PSK prefix, 7 "show on map", 7 "map labels", label gap, ADIF path, OTA program/band/mode/sort/age, watch list, Save | S (each part) / X (most) | 5–7 | No spots until a node is typed. Everything else is fine off. |
| On this computer: station hardware (not iOS) | rigctld, rotctld, ADS-B, gpsd host+port, serial GPS + baud, Save | X | 7–8 | Panes say "not set up". Fine. |
| HamAlert | user, password, Save | S | 8 | Off. |
| hams.at API key | key, Save key | X | 8 | Off. |
| Callsign lookup account | None/QRZ/HamQTH, user, password, Save | S | 8–9 | No names on spots. Fine. |

Settings that live elsewhere today, which is already the right idea:
- **Long-press a pane:** change widget, next/previous, rotation interval and set, empty, reset all.
- **Map menu:** night shade, borders, cities, gray, path, long path, overlay layers, base map, style rotation, grid, projection, centre, label mode.
- **Tap the map:** set DX or DE.

Since every setting is one tap from Settings, the problem isn't depth. It's **length and mixing**. A first-time user has to recognise "APRS-IS server (receive-only)", "Multicast group (blank = off)" and the 230-character watch-list label as things they can ignore.

### Apple TV (SwiftUI, apple/tvos)

`SettingsView.swift` already does what this proposal asks for:
- With no station, it opens a **setup mode** that shows only the Station card, with a "Later" button.
- Afterwards it shows Station plus 8 display cards.
- DX is a grid field, not coordinates.

That is the model to copy. Two differences from the Qt app:
- tvOS asks for a DX grid while Qt asks for DX latitude and longitude.
- tvOS cannot search by "Use my location".

---

## 2. What the research says

### The original HamClock (WB0OEW)

- **Setup flow.** Setup is a series of full-screen pages, six in the 2.x guide, shown at boot after touch calibration. If nobody touches them, they time out and continue. Sources: the HamClock 2.79 User Guide, [readkong mirror](https://www.readkong.com/page/hamclock-2-79-user-guide-clear-sky-institute-8429972), and a Pi walkthrough at [hamradio.my](https://hamradio.my/?p=6551).
- **Page 1** mixes everyone-settings with expert ones: callsign, DE lat/long or grid, gpsd, IP geolocation, Wi-Fi SSID and password, NTP server, DX Spider host, cluster commands, rigctld/rotctld/flrig.
- **Page 2** holds map centre, GPIO, brightness, date order, units, logging, demo, full screen, a per-day display on/off schedule, and a colour editor.
- **Editing.** Fields are typed on an on-screen keyboard. Bad fields show a bare "Err" and block Done until fixed.
- **Direct manipulation.** Users liked that the clock is changed by touching it:
  - Tap the top of a pane to pick its content.
  - Tap the map to set DX; hold to set DE.
  - Tap the DE lat/long/grid to edit it.
  
  HamClock Pro already has most of this through pane long-press, the map tap menu and the map menu.
- **Pain points.** There is far more "how to set up HamClock" material than complaints, which suggests setup itself was the hurdle. Examples: [N1FD](https://www.n1fd.org/2023/08/24/hamclock/), [DXZone guide](https://www.dxzone.com/dx36590/how-to-set-up-a-hamclock-for-your-shack-easy-guide-2025.html), [MWRS](https://www.mwrs.org.au/?p=8045). Concrete friction:
  - Calibration followed by six pages on a tiny keyboard.
  - Full screen buried on a late page.
  - Resolution fixed at build time.
  - Small hit targets: a tablet user [couldn't press a hidden button](https://reddit.sentinel-team.org/posts/1qxh8p7/snapshots/2026-02-07T00%3A42%3A50.64576Z) in a HamClock replacement.
  
  We did **not** find first-hand groups.io or QRZ threads about time zones or resets, so this document does not claim them.
- **History.** The original ended with 4.22 when WB0OEW became a silent key in January 2026 ([Ham Weekly](https://daily.hamweekly.com/2026/01/hamclock-creator-elwood-downey-wb0oew-silent-key-hamclock-to-shut-down/)). Users moving to HamClock Pro will expect its *direct* controls, not its setup pages.

### Apple guidance (verified text)

From the HIG [Settings](https://developer.apple.com/design/human-interface-guidelines/settings) page:
- Set good defaults so people can start without changing anything.
- Keep the number of settings small; many options make each one harder to find.
- **Detect information instead of asking for it.**
- **Put task-specific options**, such as showing or hiding parts of a view or filtering lists, **on the screen where the task happens**.
- Keep the settings area for general options people change rarely.
- On the Mac, Command-Comma opens Settings, and Settings reopens on the last pane viewed.

From the HIG [Onboarding](https://developer.apple.com/design/human-interface-guidelines/onboarding) page:
- Keep onboarding brief and optional.
- **Postpone non-essential setup.**
- Ask for permissions in context, when they're first needed.
- Pre-configure reasonable defaults.

### Other sources

- **Nielsen Norman Group, [Progressive disclosure](https://www.nngroup.com/articles/progressive-disclosure/):** show the few core options first and specialised ones on request. Keep it to **two levels at most**, and label the trigger clearly.
- **Ham apps:**
  - [OpenHamClock](https://github.com/HAMDevs/openhamclock) asks for callsign and grid on first run, derives lat/long from the grid, and leaves everything else to a settings panel.
  - [QLog](https://github-wiki-see.page/m/foldynl/QLog/wiki/Initial-Setup) requires only a station profile (callsign and grid) before it opens.
  - Log4OM users [complain](https://forum.log4om.com/viewtopic.php?p=8219) that the locator stays red until all 6 characters are typed. **Lesson:** accept a 4-character grid.

---

## 3. Principles

1. **Ask for two things, detect the rest.** Callsign and location are required. Everything else is derived or defaulted:
   - Time zone and grid from the location.
   - Units, 12/24 h, date order and week start from the device region.
   - Light or dark from the system.
2. **One question per first-run screen, three screens at most**, with Skip and Back.
3. **Settings next to the thing they change.**
   - Spot filters open from that pane's long-press menu.
   - Map spot visibility and labels go in the map menu.
   - DX is set by tapping the map.
   - Settings remains a second door, never the only one.
4. **Two levels, never three.** Settings home, then a section, then at most one sheet (the #683 Accounts pattern).
5. **One way to save.** A change applies when it is made or when you leave the field. Errors show under the field and keep the old value. No per-card Save buttons.
6. **Same words and order on every device.** Sections that don't apply on a platform are absent, not greyed out.
7. **Plain labels; ham jargon only where hams expect it.** For example "Turn screen black when idle", not "Screen blanking"; but "SSID" stays inside the DX cluster sheet.
8. **Never silently lose input.** If a saved file can't be read, say so (see B1).

---

## 4. First run (3 steps)

Mockups:
- [first-run-phone.png](mockups/first-run-phone.png)
- [pi.png](mockups/pi.png), top half

| Step | Screen | Notes |
|---|---|---|
| 1 | **What's your callsign?** One big field, uppercase keyboard. Next / Skip. | Skip means listener mode: no callsign, and spot logins stay off. |
| 2 | **Where is your station?** A **Use my location** button and **one field** that accepts a city, grid (4 or 6 characters), or coordinates in either decimal or D:M:S form. A small map and "FN31pr · New York (auto)" confirm the result. | Location sources: iPhone/iPad/Mac use the device location (PR #700); the Pi uses gpsd when it answers, otherwise the button is hidden. A city name goes to OpenStreetMap only on Find; grids and numbers never leave the device. The time zone is always suggested from the location. |
| 3 | **All set, W1AW.** Distance km/miles, Clock 24/12 h and Appearance, pre-set from the device region. Three tips: tap the map for DX, long-press a pane, the rest is in Settings. **Start HamClock**. | The tips replace a tutorial. |

On the **Pi kiosk**, the same three steps run at 1024×600. The #672 keyboard docks below and the confirmation map sits to the right.

**Not in first run:** DX, cluster, accounts, hardware, blanking. These come later, in context. For example, an empty DX Cluster pane says "Choose a node" and opens the DX cluster sheet.

**Cheap Monday version (S2):** skip the new wizard and show the existing Settings page in *setup mode*, as tvOS already does. Setup mode shows only the Station card, with "Later" and "Start".

---

## 5. New Settings structure

Mockups:
- [settings-phone.png](mockups/settings-phone.png): home, Station, Spots and feeds.
- [pi.png](mockups/pi.png), bottom half: sidebar with the Screen section open.
- [mac.png](mockups/mac.png): sidebar with Spots and feeds open.

| Section (home) | Contains | Moves from |
|---|---|---|
| **Station** | Callsign; Location (one field + Use my location); coordinates (read-only line); Time zone (auto, tap to override); DX target (read-only, "tap the map to change") | Station card, minus DX lat/lon, Suggest zone and DX zone |
| **Display** | Appearance; Units and clock (auto from region, override); Text size; Accent; Callsign banner (sheet); Map size; Big Clock face; Rotation (iOS) | Appearance, Distance units, Clock format, Text size, Accent, Banner, Map size, Big Clock, Rotation cards |
| **Screen** (Pi, Mac, TV) | Turn black when idle; Night window and days; TV edge inset; Exit | TV overscan, Screen blanking, Exit |
| **Accounts** | HamQTH, QRZ.com, HamAlert, hams.at, DX cluster login. **This is PR #683, kept as is.** | Three account cards |
| **Spots and feeds** | One row per source with live status, each opening one sheet: DX cluster filters and watch list, PSKReporter, On the air, APRS-IS, Meshtastic, WSJT-X UDP, ADIF log | The 40-setting "DX cluster" card (#683 renames it "Spots and feeds") |
| **Station hardware** (Pi, Mac) | Radio, Rotator, ADS-B, GPS, one sheet each with an on/off status | Hardware card |
| **Advanced** | Saved configurations (backup/restore); Demo mode; DX time zone override; map label gap; Reset all settings | Saved configurations, Demo mode, DX zone, Label gap |
| **About** | Version, data sources (collapsed list), privacy, licences | Data sources card |

**Moves out of Settings** (Settings keeps a link only where noted):
- "Show on map" ×7 and "Map labels" ×7 go to **Map menu ▸ Spots**. Today they duplicate the map menu's own layer switches; see B5.
- Per-pane filters (PSK radius and prefix, POTA program/band/mode/sort/age) are also reachable from **pane long-press ▸ Filter this list…**. The same sheet opens from Settings.

**Deleted or automatic:**

| Today | Becomes |
|---|---|
| "Suggest zone from home coordinates" button | Automatic on every location change |
| DX time zone picker + Auto | Automatic; override in Advanced |
| DX lat/lon fields | Set by tapping the map. The tvOS DX grid field could also come to Qt later. |
| Separate Find / grid / lat / lon fields (4) | 1 smart field |
| 8 Save buttons | 0. Done and Back save. |
| Units, 12/24 h, date order, week start defaults | From the device locale on first run |

### Counts, before and after

| | Today | Proposed |
|---|---|---|
| Things on the first Settings screen | 20 cards on a long scroll | **8 rows**, fits on one screen at every size |
| Fields a new user must fill | 3–5 (callsign + location fields + zone check) | **2** (callsign, location) |
| Station card inputs | 9 + 4 buttons | **3** (callsign, location, zone override) |
| Settings shown before any tap (phone) | ~80 | **~10** |
| Total settings | ~80 | ~74 (same features, minus 6 now automatic) |
| Save buttons | 8 + Done | **Done only** |
| Pi scroll length, longest section | 9.0 screens (the whole page) | ~1.3 screens (Spots and feeds) |
| Max depth | 1 (but very long) | 2: section, then sheet |

---

## 6. Ranked lists for the owner

Effort: **S** is under half a day with tests, **M** is about a day, **L** is several days. "Monday" means it's safe for the 1.0 cut if approved by Friday.

### Bugs found

| # | Bug | Repro | Fix | Effort | Monday |
|---|---|---|---|---|---|
| **B1** | **A station file that can't be read is ignored silently.** The app shows "Set up" as if new. The first Save overwrites the file, and the reason is never shown: `AppSettings::loadError()` exists but nothing displays it. | Write `station.conf` without the `appearance=` line, which is easy to do by hand or from an old helper script. Launch: Settings shows empty fields and no message. | Show the error in the Station section ("Your saved station couldn't be read: …; saving will replace it"). Also accept a missing `appearance` as `system`. | S | **Yes** |
| **B2** | **Done saves the device's time zone as the station's zone.** With no `de_zone` saved, the picker shows `timeZones.systemZone()`. `stationCard.save()` sees the zone as "changed" and saves it, so W1AW in Connecticut set up on a Mac in California, or a Pi left on UTC, gets the wrong local time and sunrise. Typing lat/lon by hand never suggests a zone either; only Find and grid do. | Station file without `de_zone` (rendered: W1AW shows "Los Angeles (UTC−7)"). Open Settings, press Done: `de_zone=America/Los_Angeles` is written. | Default to `timeZones.suggestZone(home)`, and suggest on every home-coordinate change. Keep "Change" as an override. | S | **Yes** |
| **B3** | **The callsign banner title is lost on Done.** It has its own Save, and `saveAndClose()` doesn't include it. | Type a custom title, press Done, reopen: empty. | Save the title on Done, or on editing finished. | S | **Yes** |
| **B4** | **Three save models in one card.** In "DX cluster", the WSPR and RBN switches save instantly, while UDP LAN, by-me and sets-DX wait for Save. Escape discards the latter but not the former. | Toggle WSPR and UDP LAN, then press Escape: WSPR stays on, UDP LAN reverts. | Make every switch apply instantly, as Appearance and Units already do. | S | Yes |
| **B5** | **Two switches for one map layer.** Settings "Show on map: DX" (`mapSourceVisible`) and the map menu's DX layer switch (`MapOverlays`) are independent. A spot can be hidden by either, and each screen can say "on" while the spots are hidden. | Turn the DX layer off in the map menu. Settings still says "DX: on". | Keep one switch, in the map menu. Remove the chips from Settings. | M | After 1.0 (or with S9) |
| **B6** | **Map label and label-gap chips cycle on tap with no hint.** "DX: auto" means tap to step through auto, none, prefix, call, dot; "Label gap" steps 0–100 px. | Tap "PSK: auto" four times. | Use a segmented choice in the map menu. | M | No |
| **B7** | **The cluster card's error text and Save share one row**, so on a phone a long warning squeezes the Save button. Minor. | — | Goes away with S1/S6. | — | — |

### Simplifications (ranked by benefit ÷ risk)

| # | Change | Why | Effort | Risk | Monday |
|---|---|---|---|---|---|
| **S1** | **Collapse expert cards behind section headers.** "Spots and feeds", "Station hardware", "Saved configurations" and "Demo" start collapsed: one row each with a summary ("DX cluster: connected · PSK any distance") and a chevron. Tap to expand in place. | Biggest win. On the Pi the page drops from 9 to about 3.5 screens (about 2.5 with S5 and S7) without moving a single field, and new users see only what matters. | S | Low: QML only; fields keep their ids, so `reload()` and `save()` still work. | **Yes** |
| **S2** | **First-run setup mode.** With no station, Settings shows only the Station card titled "Set up HamClock Pro", with **Later** and **Start**, as tvOS does. | First run becomes one screen, not nine. | S | Low | **Yes** |
| **S3** | **Locale defaults** for units, 12/24 h, date order and week start, applied only when `options.ini` has no value. Read from `QLocale::system()` (`measurementSystem`, `timeFormat`, `firstDayOfWeek`). | US hams get miles and 12 h without hunting, and nobody has to visit three cards. | S | Low–Med: an existing user who never touched these keys would see them change once. To avoid that, apply only when the station isn't configured yet (true first run). | **Yes** |
| **S4** | **Time zone automatic** (the B2 fix) and **remove the "Suggest zone" button**. Show "New York (auto) · Change". | One fewer decision and one fewer button. Fixes wrong local times. | S | Low | **Yes** |
| **S5** | **Group the small display cards** (Appearance, Units, Clock format, Text size, Accent, Big Clock, Map size, Rotation) into one **Display** card with labelled rows. Group TV overscan, Blanking and Exit into one **Screen** card. | Takes 11 card headers down to 2. Shorter page, clearer grouping. | S | Low | **Yes** |
| **S6** | **Split the 40-setting DX cluster card** into labelled, collapsed sub-groups: Watch list, PSKReporter, On the air, APRS, Meshtastic, WSJT-X UDP, ADIF. | People can find "APRS radius" without reading the whole card. Pairs with S1. | S | Low (QML only). Coordinate with #683, which renames the card "Spots and feeds"; rebase on it. | **Yes** (after #683) |
| **S7** | **Move Data sources to the bottom** as a collapsed "About and data sources" row. Put Exit in Screen. | 17 paragraphs of attribution stop splitting Settings in half. | S | Low (the attribution stays one tap away) | **Yes** |
| **S8** | **One Save model:** remove the 8 per-card Save buttons, so Done saves everything (it already calls each card's `save()`) and errors show in the card. Keep "Sign in" in the #683 sheets. | One rule to learn. Fixes B3 and B4 in passing. | S–M | Med: clashes with #694 ("Saved" feedback on Save buttons). Ask the owner which to keep; this doc recommends Done-only, with the #694 "Saved" toast shown on Done. | Yes, if #694 is reconciled |
| **S9** | **Move "Show on map" and "Map labels" out of Settings** into the map menu, one switch per source. | Fixes B5. Settings sits next to the thing it changes (Apple HIG). Takes 15 chips off Settings. | M | Med: MapMenu is touched by #697 (open). | Maybe; better after 1.0 |
| **S10** | **Drop DX lat/lon from the Station card**; show "DX: Tokyo · PM95 · tap the map to change", with the DX zone automatic. | DX is a moment-to-moment choice, not a setting. Removes 3 fields and a picker. | S | Low–Med: keyboard-only Mac users still need a way to type DX. Keep an "Enter DX grid…" link (the tvOS field). | **Yes** |
| S11 | **One smart location field** (city, grid or coordinates, auto-detected) replacing Find + grid + lat + lon. | 4 fields become 1 and match the first-run screen. | M | Med: needs a parser test matrix (4/6-character grids, D:M:S, comma decimals); see `docs/location-input-research.md`. | No, after 1.0 |
| S12 | **Settings home + sidebar layout** (the mockups) instead of a single scroll. | The end state: fits on one screen, and the Mac reopens on the last section. | L | Med–High this close to release | No |
| S13 | **Plain-language labels.** "Turn screen black when idle", "TV edge inset", "Commands after login" stays in the cluster sheet, "UDP spot input port" becomes "WSJT-X / JTDX spots (UDP port)". Switches get fixed labels ("Accept from other computers") instead of text that flips ("UDP from LAN: on" / "UDP from this device only"). | Clear for non-experts; switch state is shown by the switch, not by rewritten text. | S | Low | Yes (text only) |

### Enhancements (after 1.0)

| # | Idea | Effort |
|---|---|---|
| E1 | Full 3-step first-run wizard (section 4) with "Use my location": #700 on Apple, gpsd on the Pi. | M |
| E2 | **Filter this list…** in each spot pane's long-press menu, opening the same sheet as Settings. | M |
| E3 | Settings home with sidebar on Pi and Mac and push navigation on phones (S12), reopening on the last section. | L |
| E4 | "Reset all settings" in Advanced, with a two-tap confirm and the station kept. | S |
| E5 | Search box in Settings (Mac and iPad). | M |
| E6 | Status dots on section rows ("Connected", "2 signed in", "GPS: no fix"), as #683 does for Accounts. | S each |
| E7 | Bring the tvOS "DX target grid" field to the Qt app, and the Qt "Use my location" to tvOS, so platforms match. | S |
| E8 | Empty-state buttons on panes: an empty DX Cluster pane shows "Choose a node", opening the cluster sheet. Postponed setup, per the Apple HIG. | S each |

### Top 10 for Monday, in order

1. **B2/S4:** time zone follows the station automatically; remove "Suggest zone".
2. **B1:** show a station file that couldn't be read instead of starting blank.
3. **B3:** the banner title is kept on Done.
4. **S1:** expert cards start collapsed with one-line summaries (Pi page from 9 to about 3.5 screens; about 2.5 with items 6 and 9).
5. **S2:** first-run setup mode shows only the Station card (Later / Start).
6. **S5:** one Display card and one Screen card instead of 11 small cards.
7. **S3:** units, 12/24 h, date order and week start from the device region on first run.
8. **S10:** DX shown read-only with "tap the map"; DX lat/lon and DX zone fields removed.
9. **S6 + S7:** the spots card split into collapsed sub-groups (after #683); Data sources moved to the bottom.
10. **B4 + S13:** every switch applies instantly; plain labels that don't flip.

**Owner decisions needed:**
- Done-only saving (S8) versus the per-card "Saved" buttons of #694.
- Whether S3 may change units for existing users who never set them (recommendation: new installs only).
- Whether S9 (map spot switches move to the map menu) waits until after 1.0.

---

## Method and limits

- **Renders.** Qt offscreen renders on macOS, so platform-specific cards show as they do on the Mac build; on iOS, `Qt.platform.os` hides Exit, overscan, blanking, hardware, ADIF and multicast. No simulators were used.
- **Not on main yet.** PR #683 (Accounts) is not merged; the structure above assumes it lands as is.
- **Research gaps.** Community complaints about the original HamClock's setup are thin in what search returned. The cited tutorials show *that* setup took explaining, not exactly which steps hurt.
- **Mockups.** The HTML sources are in `docs/ux/mockups/src/`, rendered with headless Chrome at the device sizes shown.
