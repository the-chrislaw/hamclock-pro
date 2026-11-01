# Adaptive panes: QA pass 2

Independent test of the auto-sizing widget panes, run on Oct 29-30, 2026.

- **Build tested:** `fix/adaptive-pane-fill` (PR #701, still open) at `b89d79d`, macOS offscreen build.
- **Data:** live network data with station W1AW (FN31PR), DX Tokyo. Every widget got a warm-up render first, so its disk cache was filled before the real renders.
- **Sizes:** 800x480, 1024x600, 1280x800, 1512x982, 1920x1080, 2560x1440, 834x1210, 1210x834, 390x844 and 844x390, all dark. Light theme at 1024x600 and 1920x1080.
- **Coverage:** every widget (43 cards plus `passes`) in pane 1 at every size, and again in the station column's extra pane wherever one appears (1280x800, 1512x982, 1920x1080, 834x1210, 1210x834). That is about 570 renders. I looked at every contact sheet.

## Verdict: USABLE WITH ISSUES

On the primary hardware (Raspberry Pi 7-inch, 800x480 and 1024x600), every widget that has data renders with no text cut off. Most leave 15 % blank or less, and the type is readable. The default dashboard looks finished there, in both themes. Lists, plots and tables mostly do fill their panes now. Band conditions, HF path reliability, Daylight, Solar wind, Aurora, hams.at, Earthquakes, Balloons and Marine all look good at every size.

It does not yet meet "no matter where you put it it looks the best possible". The failures cluster in three places:

1. **Big or tall panes.** The 1512x982 and 1920x1080 pane rows and the extra panes on desktop and iPad suffer most. Here growth stops at the 1.8x cap, so single-value cards float in a band of empty space. Tables spread their rows so far apart that they stop reading as tables. Lists keep 10-11 px text in a pane that is 560 px tall. Two neighbouring panes can show body text at 11 px and 24 px.
2. **Loading, error and empty states.** These were never adapted. Each pane is left mostly blank with a lone "—" or a bare "Loading…": Satellites is 100 % blank, Launches is 80-91 % blank, PSKReporter shows "HEARD BY —" and Passes shows "Waiting…" in giant grey type. Satellites is a default pane at 1920x1080 and on iPad, so a fresh install with a slow CelesTrak response shows a big empty box.
3. **The 2560x1440 layout.** About 750 px of the station column sits empty under the station card.

None of these break the app, which is why the verdict is not NOT USABLE. But a user on a desktop or iPad will notice items 1-5 below within seconds.

## Top 10 problems (ranked by how much a user would notice)

Paths are relative to `docs/qa/adaptive-pass2/`.

1. **2560x1440: the station column is empty below the station card (about 268x750 px).** The grid takes all 12 saved slots, so the spill pane never appears (`spillIndex < appSettings.panes.length` is false). The station card stretches to full height with nothing in it.
   Screenshot: `dashboards/2560x1440-dark.jpg`.
   Fix: let the spill use a 13th/14th slot (raise the slot limit by the spill count), or give the grid one slot fewer when the column has room for a pane. As a fallback, put the Big Clock or a second station block there.
2. **Loading and error states look broken in large panes.**
   - Satellites (live) is a blank card with "Loading…" in the footer, 100 % blank at every size. CelesTrak timed out from the test machine (curl gave up after 75 s), and the AMSAT fallback did not populate it within the screenshot delay.
   - PSKReporter (offline) shows "HEARD BY" over a lone "—".
   - Launches (offline, and also live with the fixture's past dates) shows "NEXT LAUNCH (NET) —", 80-91 % blank.
   - Passes shows "Waiting for satellite elements" in about 26 px grey text, repeated word for word in the footer.

   Screenshots: `dashboards/1920x1080-dark.jpg` and `dashboards/834x1210-dark.jpg` (Satellites as the default spill pane), plus `sheets/1920x1080-dark-spill-2.jpg`.
   Fix: give these states the same treatment as `needsSetup`, at card level. Show a centred icon and reason, with a Retry or Open Settings button while `state` is `loading` or `offline` and the model has no rows, and hide the widget's empty skeleton. Say "No launches scheduled" instead of "—".
   With fixtures, Satellites and Passes look good (`fixtures/1920x1080-dark-spill.jpg`). The gap is only the state handling.
3. **Uneven type scale between neighbouring panes.** Each pane picks its own fit, so the same text role renders at very different sizes side by side. At 1920x1080, a row can hold On the air at about 11 px, Tropical storms body text at about 24 px and Space alerts at about 26 px. At 1024x600, Space weather values are about 18 px next to Space alerts at about 26 px. The dashboard looks unbalanced.
   Screenshots: `sheets/1512x982-dark-pane1-3.jpg`, `sheets/1920x1080-dark-spill-2.jpg`, `sheets/1024x600-dark-pane1-2.jpg`.
   Fix: cap the growth of text-only content at a dashboard-wide body size. For example, compute one ceiling per screen from the median pane fit, or lower `maxGrow` to about 1.35 for content whose main element is body text. Spend the leftover height on more detail rather than bigger letters (see 8).
4. **Lists in big panes keep small text.** Content that stretches is only scaled up to its design width, so a long list never grows.
   - On the air: 25 rows at about 10-11 px in the 340x560 spill pane at 1920x1080, and in the 326x462 pane at 1512x982.
   - Planets in the iPad portrait spill pane: about 9 px.

   This reads poorly at desk distance next to 20+ px headlines elsewhere.
   Screenshots: `sheets/1920x1080-dark-spill-2.jpg`, `sheets/834x1210-dark-spill-2.jpg`.
   Fix: for lists, grow until row height reaches a target (about 1.6x body) before adding rows. A useful rule: once more than about 12 rows fit at fit 1, prefer bigger rows over more of them.
5. **Planets grows in some panes and not others.** It is large and filled at 1024x600 and 2560x1440. It stays small with 34-41 % blank at 1512x982, 1920x1080, 1210x834 (spill) and all spill panes. On iPhone portrait it shows only two of the three planets.
   Screenshots: `sheets/1512x982-dark-pane1-2.jpg`, `sheets/1210x834-dark-spill-2.jpg`.
   Fix: mark it `scaleToFill`, or list every body (Moon, Mars, Jupiter and so on), with those below the horizon greyed out, so the list fills the pane.
6. **The Dst index plot does not fill its pane.** The plot sits in a fixed band with a large blank gap before the captions: 46 % blank at 800x480, 32-40 % at 1512x982, 1920x1080 and spill panes, 44 % at 844x390.
   Screenshots: `sheets/800x480-dark-pane1-3.jpg`, `sheets/1512x982-dark-spill-3.jpg`.
   Fix: give the plot `Layout.fillHeight` (contract rule 2), as Solar wind and Aurora do.
7. **Tables in tall panes spread their rows until they fall apart.** At 1920x1080 (pane and spill) and 1512x982, rows are 60-90 px apart in Next eclipse, Space weather impact, NOAA scales, D-RAP, HamQSL and Beacons. The eye loses the label-to-value pairing.
   Screenshots: `sheets/1920x1080-dark-spill-1.jpg` (Next eclipse, Beacons), `sheets/1512x982-dark-spill-3.jpg` (Space weather impact).
   Fix: enforce the contract's "cap each row at about three lines of body text". Clamp row pitch to about 2x line height, then centre the block, or use the room to grow the type.
8. **Single-value cards are poster-sized and float in blank bands.**
   - Fire weather "None active" is about 40 px.
   - Space alerts is one alert set in a 26 px headline.
   - Host sensors shows "90 % used" under a "CPU TEMP —" placeholder.
   - APRS "0 Listening" leaves 40-60 % blank at 1280x800 and 1210x834.
   - Fires at 1920 spill.

   In tall panes these leave 25-40 % blank above and below, even at the growth cap.
   Screenshots: `sheets/1920x1080-dark-spill-1.jpg`, `sheets/1920x1080-dark-spill-2.jpg`.
   Fix: in `tall` or `roomy` panes, show the secondary detail these models already have: recent alerts, per-volume disk use, a top-fires list, an APRS station list. Otherwise top-align the content and keep the headline at 1.3-1.4x.
9. **Weather location labels wrap badly or vanish.**
   - "DE · HARTFORD, US" wraps over two or three lines in panes 212-355 px wide (800x480, 1024x600, 1280x800, 2560x1440, 844x390).
   - The DX place name disappears at some sizes: it reads "DX" only at 2560x1440, 1024x600 and on iPhone, but "DX · SHIBUYA, JP" elsewhere.

   Screenshots: `sheets/1024x600-dark-pane1-3.jpg`, `sheets/2560x1440-dark-pane1-3.jpg`.
   Fix: put the place name on its own elided line under "DE" and "DX", and always show it.
10. **iPhone details look unfinished.**
    - HF path reliability on 390x844 collapses to one line of tiny red "80m 0% 40m 0% …" with 26 % blank. The grid that makes the widget useful is gone.
    - Sun (SDO) on iPhone is a 100 px image with the "AIA 193 Å" caption running over its corner. The label also overlaps the image's top-left corner at every size, which is most visible in the light theme.

    Screenshots: `sheets/390x844-dark-pane1-1.jpg`, `sheets/390x844-dark-pane1-3.jpg`, `sheets/1024x600-light-pane1-3.jpg`.
    Fix: show a small heat strip (8 rows by 24 hours) on phones, and put the image caption above the image.

### Minor

- **Sun (SDO) at 2560x1440:** the image fills only about half the pane width, which leaves the right side empty (`sheets/2560x1440-dark-pane1-3.jpg`).
- **Space weather sparkline:** the axis letter "B" is overdrawn by the plot line at every size.
- **Balloons in the iPad portrait spill pane:** the last row is elided ("WxR-301D 23053…").
- **Possible false positives from the truncation checker:** `TRUNCATED … (clipped)` was reported for the station card's sunrise/sunset lines at 1024x600 and 844x390, yet those lines render complete. If that is right, it is noise that could hide a real clip in CI.
- **Shape switching:** the same widget switches between its compact one-line layout and its stacked layout between nearly identical panes. Examples: NOAA scales shows "R0 none" at 266x187 but the full table at 277x234; Space weather is compact at 277x234 and 355x299 but stacked at 279x366. Each rule is defensible, but on screen it looks inconsistent.

## Full dashboards

| Size | Use of space | Notes |
|---|---|---|
| 800x480 | good | Three panes fill well. On the map, Antarctica is cropped (outside pane scope). |
| 1024x600 | good | Default panes look finished in both themes. |
| 1280x800 | good | Band conditions spill pane matches the top row. |
| 1512x982 | good | Tall panes switch to the stacked layout. Sun & Moon spill pane is well filled. |
| 1920x1080 | acceptable | The Satellites spill pane (340x560) is blank while loading (problem 2). |
| 2560x1440 | poor | Empty station column (problem 1). Text is small for the screen size (about 12 px body). |
| 834x1210 (iPad portrait) | acceptable | Two spill panes stack well. Satellites is blank. The map shows the Pacific view, with DE at the right edge. |
| 1210x834 (iPad landscape) | good | |
| 390x844 (iPhone portrait) | good | Paging dots work. The Space weather chart is a flat strip. |
| 844x390 (iPhone landscape) | good | |

Screenshots for every size are in `dashboards/`.

## Ratings per widget and size

**Key:** G = good, A = acceptable, P = poor. "Spill" is the station column's extra pane at 1280x800, 1512x982, 1920x1080 and on iPad (the worst of these is shown). The column headers are the sizes 800x480, 1024x600, 1280x800, 1512x982, 1920x1080, 2560x1440, 834x1210, 1210x834, 390x844 and 844x390.

The unconfigured widgets are adif, adsb, bandactivity, dxcluster, gps, hamalert, lookup, mesh, radio and rotator. All of them show a centred reason and an Open Settings button, which looks intentional at every size: **G everywhere**.

| Widget | 800 | 1024 | 1280 | 1512 | 1920 | 2560 | iPad P | iPad L | iPhone P | iPhone L | Spill | Reason for any A/P |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| aprs | A | A | P | A | A | A | A | A | A | A | P | Empty or one-station state. "0 Listening" is huge at 800x480. A single row leaves 51-60 % blank. |
| balloons | G | G | G | G | G | G | G | G | G | G | A | Last row elided in the iPad spill pane. |
| bands | G | G | G | G | G | G | G | G | G | G | G | |
| bandsmatrix | G | G | G | G | G | G | G | G | P | G | G | Phone portrait loses the grid (problem 10). |
| beacons | G | G | G | G | A | G | G | G | G | G | A | Rows spread 65-80 px apart in tall panes. |
| daylight | G | G | G | G | G | G | G | G | G | G | G | |
| eclipse | G | G | G | A | A | G | G | G | G | G | A | Rows spread too far (problem 7). |
| eme | G | G | G | G | G | G | G | G | G | G | G | |
| fires | G | G | G | G | A | G | G | G | G | G | A | Centred with blank bands when tall. |
| firewx | A | A | A | A | A | A | A | A | A | A | A | "None active" is poster-sized (problem 8). |
| hamsat | G | G | G | G | G | G | G | G | G | G | G | |
| launches* | P | P | P | P | P | P | P | P | P | P | P | Offline or empty state is a lone "—" in a 78-91 % blank pane (problem 2). |
| marine | G | G | G | G | G | G | G | G | G | G | G | |
| ota | A | G | G | G | A | G | G | G | G | A | P | Headline only at 800x480 and 844x390. Rows about 10 px in big panes (problem 4). |
| passes* | P | P | P | P | P | P | P | P | P | P | P | Live data still loading: giant grey "Waiting…", repeated in the footer. With fixtures: G at all sizes. |
| planets | A | G | G | P | P | G | A | P | A | G | P | Does not grow in tall or narrow panes. Two of three planets on iPhone (problem 5). |
| pskreporter* | P | P | P | P | P | P | P | P | P | P | P | Error state "HEARD BY —". |
| quakes | G | G | G | G | G | G | G | G | G | G | G | |
| satellites* | P | P | P | P | P | P | P | P | P | P | P | Live data still loading: 100 % blank. With fixtures: G (A at 1024x600, where the polar plot is small). |
| sensors | A | A | A | A | A | A | A | A | A | A | A | "CPU TEMP —" placeholder on Mac. "DISK" is poster-sized. |
| solaralerts | A | A | A | A | A | A | A | A | A | A | A | Small at 800x480 (40 % blank), 26 px headline elsewhere. |
| solaraurora | A | G | G | G | G | G | G | G | A | A | G | The compact text-only layout leaves 34 % blank. |
| solardrap | A | G | G | G | G | G | G | G | A | A | G | Compact layout, 22-34 % blank. |
| solardst | P | A | A | P | P | G | A | A | A | P | P | Plot does not fill (problem 6). |
| solarhamqsl | G | G | G | G | G | G | G | G | G | G | G | |
| solarimage | G | G | G | G | G | A | G | G | A | G | A | Image at half width at 2560x1440. Tiny image with overlapping caption on iPhone. |
| solarimpact | G | G | G | A | A | G | G | G | G | G | A | Rows spread too far (problem 7). |
| solarscales | G | G | G | A | A | G | G | G | G | G | A | Rows spread too far. |
| solartrend | G | G | G | G | G | G | G | G | G | G | G | |
| solarwind | G | G | G | G | G | G | G | G | G | G | G | |
| spaceweather | G | G | G | G | G | G | G | G | G | G | G | The sparkline overdraws the "B" axis label (minor). |
| storms | G | G | G | A | A | G | G | G | G | G | A | Body text about 24 px, oversized next to its neighbours (problem 3). |
| sunmoon | G | G | G | G | G | G | G | G | G | G | G | |
| weather | A | A | A | G | G | A | G | G | A | A | G | Location label wraps or vanishes (problem 9). |

\* Live data was unavailable from the test machine for these four widgets, so their ratings are of the state handling, not of the data layout:

- **satellites and passes:** CelesTrak timed out.
- **pskreporter:** unreachable.
- **launches:** unreachable during the main run. In an earlier live render at 2560x1440 it showed data and looked good.

The fixture renders in `fixtures/` show the layout with data.

**Measured blank space.** This is the largest blank band in the card body, from `--check-fill 0`, for widgets with data.

- **At most 15 % at every size:** bands, daylight, solarwind, hamsat, marine, quakes, balloons, eme, sunmoon.
- **Worst:** satellites (100 %, loading), launches (78-91 %), pskreporter (37-47 %), solardst (up to 46 %), planets (up to 41 %), solaralerts (up to 40 %), aprs (up to 60 %).

## Files

`docs/qa/adaptive-pass2/` holds:

- **`dashboards/`:** the full dashboard at every size, plus the light theme at 1024x600 and 1920x1080.
- **`sheets/`:** each widget's pane at native size, with 16 panes per sheet. File names are `<size>-<theme>-<pane1|spill>-<n>.jpg`. Each tile is labelled with the widget id, its state, the pane size, the blank % and a `T<n>` truncation count.
- **`fixtures/`:** Satellites, Passes, Launches and PSKReporter rendered with their test fixtures.
