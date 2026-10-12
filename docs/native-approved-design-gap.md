# Translate the approved dashboard into native Qt

October 3, 2026. This plan follows the user's explicit correction: the approved refined mockup is the visual target; the sparse native dashboard is a functional integration increment, not acceptance of a different design.

## Evidence inspected

Viewed both `outputs/hamclock-pro-refined-light.png` and `outputs/hamclock-pro-refined-dark.png`, and read `outputs/hamclock-pro-refined.html`, `design/dashboard.html`, the parent Apple design research/design-reset/dashboard UX review and current `src/qt/station_dashboard.cpp`. The screenshots are 1280×800 and demonstrate the accepted composition. Repository HTML has later accessibility/reflow changes; use its interaction principles without treating demonstration measurements or hand-drawn geography as production data.

Apple's [UI design guidance](https://developer.apple.com/design/tips/) supports screen-fitting primary content, aligned related elements, nearby controls, readable text, sufficient contrast and 44-point touch controls. Those are principles; the exact native pixel geometry below is our project proposal. Apple points do not directly establish physical Pi hit size. The [Layout HIG page](https://developer.apple.com/design/human-interface-guidelines/layout) still returned only a JavaScript shell in this run. Existing research remains context, not evidence that the native design implements it. No Apple fonts, symbols, artwork or templates should be redistributed.

## Candid gap assessment

| Approved target | Current native integration | Required correction |
|---|---|---|
| Minimal masthead; identity and dominant UTC inside left station card | UTC moved into the masthead beside brand | Restore station-first clock hierarchy, with masthead reserved for quiet appearance/settings actions |
| One grouped left surface containing station, DE, DX and independent spots area | One generic scroll area containing bold headings and coordinate labels | Distinct inset sections and fine separators; station/clock and selected endpoints stay visible while observations scroll |
| Three upper instrument panes plus narrow space-weather utility strip | Entire upper strip absent | Restore the structural strip immediately, with truthful unavailable states pending provider work |
| Dominant rounded map card, slim toolbar, geographic land fill and selected-path summary | Bare letterboxed coastline with text below | Map card, filled licensed geography, toolbar, restrained graticule and nearby concise path context |
| Warm-neutral light / slate-blue dark surfaces | Shared editor green palette | Dedicated dashboard semantic tokens matching the accepted study; preserve editor functionality without letting its palette dictate all dashboard aesthetics |
| Measured size hierarchy and smooth aligned typography | Inherited platform sizes plus arbitrary increments | Explicit role scale and aligned measurements; changing numbers must not move surrounding UI |

The native screen is operationally useful but does not yet express the approved visual system. Passing code and accessibility-interface tests did not validate visual fidelity.

## Native semantic tokens

Take these exact base colors from the approved HTML. They are reference values, not a contrast/compliance certificate. Preserve semantic roles so measurements can adjust a role without recoloring unrelated items.

| Role | Light | Dark |
|---|---|---|
| Canvas | `#eeede9` | `#22282c` |
| Card surface | `#fafaf7` | `#2b3236` |
| Primary text | `#303a42` | `#e2e7e6` |
| Secondary text | `#637074` | `#97a4a8` |
| Divider | `#e0e3df` | `#41494b` |
| Selected/focus accent | `#487991` | `#9cb6c5` |
| Ocean | `#dce8eb` | `#374a51` |
| Land | `#f3f3ec` | `#596465` |
| Coast | `#bac9ca` | `#6f8587` |

Use primary/secondary text for real data. Pale separators can group noninteractive sections; essential control boundaries and focus indicators require separately measured contrast. Error/warning roles need text or symbols, not color alone. Use opaque surfaces and no animated glow/blur. Native map route/marker colors must be checked against both ocean and land; keep DE circle versus DX square semantics even though the concept uses two circles. Do not replicate low-contrast micro-labels just because the screenshot contains them.

## Card geometry and arrangement

**Reference 1280×800:** outer horizontal margins 32, top 26; quiet masthead about 28 high then 21 gap. Main composition spans approximately x32–1248, y74–748. Left column 190; gutter 16; workspace 1010. Left card and both workspace cards radius 13, one fine perimeter each. Upper strip 175 high; inter-card vertical gap 16; map consumes all remaining height. Instrument widths follow 1.05:1:1 plus utility width 82, divided internally with fine vertical rules. Use 17–20 padding in main instruments, 13 in utility; left sections use 20 horizontal padding. No nested outlines around each statistic.

**Compact 800×480 proposal:** outer margins 12 horizontal / 8 vertical; masthead 44 high for real touch targets; gap 8; optional status/footer 16 with gap 8. Remaining composition height 388. Left column 166, gutter 12, workspace 598. Upper strip about 112–132 depending text/input mode; gap 8; map gets the remaining 248–268. Compact card radius 9, padding 10–12, separators aligned across sections. This is a proposed native adaptation, not a claim that a screenshot reduced uniformly will be usable. If content does not fit at readable sizes, use the accessible secondary-view/reflow rules below rather than shrinking text further.

**Left composition:** station caption → callsign → optional resolved station name/grid → real UTC clock with smaller seconds → date/UTC label. Next, DE and DX sections each show current authoritative coordinates and grid. Local time, sunrise/set and city names are absent until computed or resolved by a validated source; retain a concise unavailable label or omit that optional subrow. Below, keep a DX cluster region with connection/empty/error state, independently scrollable when real spots exist. Keep Edit station reachable as a compact visible control near the station/DE context, not a large last row hidden behind a full sidebar scroll. If height is insufficient, move edit/details into a station-header menu with the same accessible action name and tested shortcut/focus route.

**Upper composition:** left time-series pane (initial Solar flux); middle Propagation; third The Sun; narrow Space weather utility. Pane headings are distinct from data and carry meaningful selection affordances only when choices are implemented. Available provider states must eventually show source/observation time. Unavailable content retains the intended visual footprint with a short reason, for example “No solar flux data” or “Propagation model unavailable”. Do not draw a plausible curve, favorable colored prediction cells, sunspots or numeric zero as a missing-data substitute. No-op chevrons or interactive-looking disabled fake controls are unacceptable.

**Map composition:** independent rounded surface with 44-high touch toolbar (24–32 mouse mode) and real map title; operational layers/actions only. Route toggle can work now; Day & night must be clearly unavailable until true terminator calculation exists. Keep selected-path summary associated with map, as a small opaque inset or reserved bottom row. Show actual endpoints using DE/DX or saved labels and calculated distance/bearing, with undefined-bearing cases. No sample “20 m” band without a selected band source. Attribution lives in a discreet info/credits location with a compact visible label where required.

## Typography and spacing

Use one antialiased system sans family for local builds; record a redistributable font's actual license before bundling for Pi. Do not copy the Mac system font files. Use explicit Qt role sizes in logical pixels rather than arbitrary offsets from whatever platform default happens to load; account for device scaling and inspect real rendered output. Body/values regular, titles medium, callsign semibold. Avoid broad all-caps tracking; short DE/DX identifiers can remain uppercase.

| Role | Reference logical px | Compact logical px |
|---|---:|---:|
| Brand | 17 | 15 |
| UTC hours/minutes | 34–38 | 28–32 |
| UTC seconds | 17 | 14–16 |
| Callsign | 27 | 20–23 |
| Instrument main value | 31 | 24–26 |
| Location primary | 17 | 14–15 |
| Pane title / data row | 13–14 | 12–13 |
| Secondary/unit/state | 12–13 | 11–12 |

Reference CSS used some 8–10px labels; do not blindly preserve those on a physical seven-inch display. Tabular number support must be verified in the actual chosen family, or reserve stable widths for changing clocks/numbers. Use 4/8/12/16/24 spacing steps, consistent label baselines and right-aligned numeric columns. Wrap meaningful errors but do not let state labels force unbounded card growth.

## Real geographic fill and legal data

Existing Natural Earth 110m coastline linework is a licensed-provenance success, but line paths are not land polygons. Do **not** fill its disconnected polyline fragments or connect dateline fragments into artificial continents. Next map batch should ingest [Natural Earth 110m land polygon data](https://www.naturalearthdata.com/downloads/110m-physical-vectors/110m-land/) from the official distribution using an independently written bounded importer. Pin the fetched bytes/hash, source URL and [public-domain statement](https://www.naturalearthdata.com/about/terms-of-use/); preserve polygon parts/holes, dateline seams and latitude bounds. It is a new data/projection audit, not permission to ship an unreviewed downloader. No dependency installation or asset copying from the mockup is needed for this plan.

Keep equirectangular projection correctly labeled for the current implementation. Accepted concept geography is approximate and “Mercator-style”; visual similarity does not justify relabeling an equirectangular map or stretching its aspect ratio. Fill card background with ocean, keep geographic viewport/projection internally consistent, and allow restrained letterboxing where necessary. Future Mercator/Robinson projections need explicit inverse/forward contracts and seam/pole tests. Land fill then thin coast, faint/toggleable graticule, route and distinct markers establish the intended hierarchy. No decorative routes to arbitrary continents.

## Small-screen interaction and reflow constraints

- The normal 800×480 kiosk view must expose station identity/time, DE/DX, the top-strip structure and map without horizontal scrolling. Respect actual client area: window decorations can exceed a nominal 800×480 desktop window; test kiosk/fullscreen separately before claiming screen fit.
- Keep minimum 44×44 logical activation regions in touch mode; measure physical size on the seven-inch panel later. Text or inactive labels do not all need 44px rows. Align invisible hit regions with visible controls without overlaps.
- Preserve logical keyboard order from station actions to pane actions, map actions and appearance; focus must be visible on both themes. Opening a menu/editor moves focus predictably and closing it returns focus to the initiating action. Test the actual native button layout and live System-theme changes.
- Larger-text mode or width below 700 must reflow into readable sections with vertical scrolling/secondary views. Preserve all features through visible navigation rather than clipping, hiding or forcing tiny labels. Do not reuse the old HTML `min-width:700px` rule that produced horizontal overflow.
- Only spots/data lists scroll in the normal composition. If compact height cannot fit all context, use explicit station-detail/utility views; ensure screen readers can discover them. No full sidebar scroll that hides clock/selected destination during ordinary spot browsing.
- Repaint changed values only; map geometry caches survive clock ticks. No continuous animation by default. Screen-reader names must expose current actual values; styling must not regress this recently repaired behavior.

## Bounded implementation and acceptance sequence

1. **Approved shell translation:** dedicated dashboard tokens/type/layout, grouped sidebar and complete instrument/utility strip with truthful unavailable states, map card/toolbar and actual saved values. Keep persistence and computed routes unchanged. Before publishing, compare computer-captured light/dark native screens beside both accepted references at 1280×800 and 800×480. Reviewer must confirm composition, clock hierarchy, no clipping, legible states and absence of fabricated metrics. Unit tests should exercise geometry/reflow and preserve modal Save/Cancel/error invariants.
2. **Real filled map:** bounded polygon importer/provenance, holes/seams/poles, fill/outline ordering, endpoint/route visibility. Independently test tiny/large maps and dateline/cross-hemisphere cases, then inspect built map through computer control in both themes. Keep source dataset and licenses auditable. Styling-only snapshots are insufficient to prove geometry.
3. **Operational pane model:** source-specific loading/empty/stale/error/readings, pane menus and truthful selected-layer state. Add one real data feature per validated batch; compare source units/time/limits and record provider permissions first. No “all features complete” claim from visible placeholder cards.
4. **Physical target gate:** actual Pi 4 seven-inch touch, Pi 3 performance, selected font rendering and onscreen-keyboard usability, startup/memory/CPU/latency. Desktop rendering does not satisfy this gate. Legal/security and accessibility audits remain integrated with every increment.

Record visual regressions and untested cases separately from passing unit checks. The next code batch should implement step 1, then queue step 2; it should not start another aesthetic direction or another standalone redesign mockup.
