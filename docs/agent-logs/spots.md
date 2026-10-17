# Agent log: spots

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (run 1)
- Step 1: no mergeable auto-merge PRs (#73 has conflicts, #60 Apple jobs cancelled, #83 CI running; all other areas', left alone).
- Backlog item 13 (slice): cluster login with SSID and post-login commands. `TelnetClient::start` takes commands, sent once after the first post-login line; `sanitize_commands` keeps only short printable receive-only commands (refuses dx/announce/talk/...). AppSettings gets `cluster/ssid` and `cluster/commands` (minimal, backward compatible) and `saveClusterLogin`; Settings page gets two fields in the DX cluster card (not screenshotted: settings page has no screenshot path; not validated on device).
- Next: watch list highlights (item 4), map overlay once area G's API exists, then POTA/SOTA.

## 2026-10-13 (run 2)
- Step 1: nothing of mine open; #92 (space) CI still running, #73 not mine, left alone.
- Backlog item 3 (slice): new `src/widgets/ota/` with POTA and SOTA parsers, merged/deduplicated list, QRT dropped, `OtaModel` over two PolledSources (live when either feed is fresh, status names a missing feed), compact card. Data source policy recorded in the roadmap. Tests and ASan/UBSan pass offline.
- Not done: map overlay (waits for area G's API), band/mode/age filters, WWFF/IOTA/etc. The card is not on the default dashboard, so no widget screenshot was possible (dashboard layout is area A's); not validated on device.
- Next: watch list highlights (item 4), PSKReporter (item 2).

## 2026-10-14 (run 3)
- Step 1: nothing of mine open; #102 (space) CI running and #73 not mine, left alone.
- Backlog item 4 (slice): watch list. Header-only matcher `src/widgets/watchlist/watch_match.hpp` (calls, `*`/`?` prefixes, /P and EA8/ parts ignored, sanitised, capped at 200). AppSettings gets `watch/list` + `saveWatchList` (minimal, backward compatible; header-only so the core library needs no new source). DX cluster rows get a `watched` role, ★ and accent colour, plus a ★ count in the card header; Settings DX cluster card gets a watch list field. Tests pass normally and under ASan/UBSan.
- Not done: DXCC-entity watching (needs item 5 prefix table), alert badge outside the card, other panes (OTA). Screenshots at 800x480/1920x1080 only show the unconfigured/connecting state since the cluster is network-only; not validated on device.
- Next: PSKReporter (item 2), prefix to DXCC table (item 5).

## 2026-10-14 (run 4)
- Step 1: nothing of mine open; #110 (space) and #73 not mine, left alone.
- Backlog items 3/14 (slice): POTA activator spots on the map through area G's overlay API (`ota_overlay`, layer `ota`; green dots, labelled by call). SOTA spots carry no coordinates so they are not drawn (a summit table would be needed). The layer is published only while the feed is live and removed when stale/offline; `OtaModel::setMapVisible` is the per-source on/off hook (Settings toggle not wired yet). Tests cover live, cap, hide and stale removal; ASan/UBSan clean. Not validated on device or screenshotted (card/map composition belongs to areas A/G).
- Next: PSKReporter (item 2), prefix to DXCC table (item 5, which unlocks DX cluster spots on the map).

## 2026-10-14 (run 5)
- Step 1: nothing of mine open; #73 still conflicting and #126 (space) has iPad CI running, both not mine, left alone. No open `[C]` qa issues.
- Backlog item 2 (slice): new `src/widgets/pskreporter/`: XML parser (malformed rows skipped, newest first), query URL builder (callsign validated, `/` encoded, appcontact), `PskModel` over one PolledSource at 5 min (PSKReporter's requested rate) that is recreated when the station callsign changes and stays Not configured while it is empty. Compact card. Data source policy recorded in the roadmap (probe from the sandbox returned HTTP 200). Tests pass normally and under ASan/UBSan.
- Not done: map overlay (needs locator to lat/lon and a label policy), "heard by me / I heard" direction, band statistics, farthest-reception distance. The card is not on the default dashboard, so no screenshot; not validated on device.
- Next: prefix to DXCC table (item 5, unlocks DX cluster spots on the map), PSKReporter map paths, HamAlert.

## 2026-10-14 (run 6)
- Step 1: nothing of mine open; #134 (space) CI still running and #73 not mine, left alone. No open `[C]` qa issues.
- Backlog items 2/14 (slice): PSKReporter receivers on the map through the overlay API (`psk_overlay`, layer `psk`; one blue dot per receiving station at its 4/6-character grid-square centre, labelled by call, short path from DE; duplicates and 2-character grids skipped; cap 30). Published only while the feed is live, removed when stale/offline/unconfigured; `PskModel::setMapVisible` is the per-source on/off hook (Settings toggle not wired). The widget library now also compiles `src/domain/locator_input.cpp` (no shared CMake file touched). Tests pass normally and under ASan/UBSan; not screenshotted (card is not on the default dashboard) and not validated on device.
- Item 5 (prefix to DXCC table) still blocked: country-files.com (cty.dat) is not reachable from the sandbox (proxy 403) and an invented table is not acceptable. Needs the domain allowlisted (owner input).
- Next: HamAlert, DXCC table once cty.dat is reachable.

## 2026-10-14 (run 7)
- Step 1: nothing of mine open; #140 (map) CI running and #73 not mine, left alone. Only open `[C]`-less qa issue is #120 (area B).
- Backlog item 15 (slice): new `src/widgets/adif/`: ADIF parser (length-based, case-insensitive tags, type indicators, bad tags/records skipped, record cap), `AdifModel` reading a local file with QFileSystemWatcher reload (64 MB cap, read-only, unconfigured while the path is empty), `worked(call)` that ignores /P and EA8/ parts, compact card. AppSettings gets `adif/path` + `saveAdifPath` (minimal, backward compatible); Settings DX cluster card gets a path field. Tests pass normally and under ASan/UBSan. Tests use synthetic ADIF, not a real log.
- Not done: not-in-log watch keywords (NADXCC/NAPref/NABand/NAGrid; need item 5 prefix table), marking worked calls in cluster rows, map overlay. Card is not on the default dashboard, so no screenshot; not validated on device.
- Next: HamAlert, UDP spot input; DXCC table still needs cty.dat allowlisted.

## 2026-10-14 (run 8)
- Step 1: merged #148 (docs, all checks green); #149 and #153 CI still running, #73 conflicting, all other areas', left alone. No open `[C]` qa issues.
- Backlog item 7 (slice): OTA display filter (`ota::filter` by program, band, mode, max age; band names shared with the DX cluster via `band_for_khz`). `OtaModel::setFilter` applies it to the list, the count and the map layer. Tests cover each criterion; normal and ASan/UBSan runs pass. No QML or AppSettings change, so no screenshots; Settings controls not wired yet.
- Next: wire OTA filters in Settings, WWFF/IOTA programmes, HamAlert, UDP spot input; DXCC table still needs cty.dat allowlisted (#136).

## 2026-10-14 (run 9)
- Step 1: nothing of mine open; #164 (space), #161 (map) CI still running and #73 not mine, left alone. No open `[C]` qa issues.
- Backlog item 12 (slice): UDP spot input. `parse_udp_spot` accepts N1MM+/DXLog-style `<spot>` XML (flat elements only; DTD/entities rejected, 4 KiB cap, call and frequency validated, implausible timestamps replaced by arrival time) and plain "DX de" lines. `UdpSpotListener` binds only when the user sets a port in Settings: 127.0.0.1 by default, all IPv4 only with the "Accept from LAN" box; no multicast. Spots join the DX cluster list. AppSettings gets `udp/port` (0 = off, else 1024-65535) and `udp/lan` (minimal, backward compatible); Settings DX cluster card gets the two controls. Tests pass normally and under ASan/UBSan (LeakSanitizer only reports libproxy allocations from Qt's system-proxy lookup in this sandbox, shared with the existing QML tests; the new socket opts out of proxies).
- The XML field names follow N1MM's documented spot broadcast but are not verified against a real capture; tests use synthetic datagrams. Not validated on device; Settings controls not screenshotted (no direct way to render the page).
- Not done: WSJT-X binary protocol, multicast, "UDP sets DX", by me/all filter, Log4OM format.
- Next: HamAlert, OTA filter controls in Settings, WWFF/IOTA; DXCC table still needs cty.dat allowlisted (#136).

## 2026-10-14 (run 10)
- Step 1: nothing of mine open; #173 (space) CI running and #73 not mine, left alone. No open `[C]` qa issues.
- Backlog item 16 (slice): `parse_ota_ref` pulls one WWFF/SOTA/IOTA/POTA reference out of a spot comment (`DxSpot::ota_program`/`ota_ref`). Strict boundaries avoid matching inside longer tokens; a POTA-shaped `XX-NNNN` is accepted as POTA. Tests pass normally and under ASan/UBSan. No QML change (reference not shown yet), so no screenshots; not validated on device.
- Not done: IOTA reference names (needs the IOTA list, a data source to vet), showing the reference in the card or map label.
- Next: HamAlert, OTA filter controls in Settings, WWFF/IOTA programmes; DXCC table still needs cty.dat allowlisted (#136).

## 2026-10-14 (run 11)
- Step 1: nothing of mine open; #181 (space) and #73 not mine, left alone. No open `[C]` qa issues.
- Backlog item 8 (slice): HamAlert pane. `TelnetClient::start` takes an optional password, sent once after a `password:` prompt (online only afterwards; printable ASCII without spaces, else the client stays idle). `HamAlertModel` connects to hamalert.org:7300 and shows incoming `DX de` spots; Not configured until user and password are set. AppSettings gets `hamalert/user` + `hamalert/password` in credentials.ini (0600), `saveHamAlert`, password C++-only; Settings gets a HamAlert card; pane chooser lists it under DX. Tests pass normally and under ASan/UBSan (leak check off).
- Not verified: the live service's exact prompts and line format (port 7300 is not reachable from the sandbox; tests use a fake socket). No map overlay, no card screenshot (not on the default dashboard), not validated on device.
- Next: HamAlert map overlay, OTA filter controls in Settings, APRS-IS, WWFF/IOTA; DXCC table still needs cty.dat allowlisted (#136).

## 2026-10-14 (run 12)
- Step 1: nothing of mine open; #186 (apple, macOS universal failing), #189/#190 CI running, #73 conflicting, all other areas', left alone. No open `[C]` qa issues.
- `www.country-files.com` is now reachable from the sandbox (#136), so backlog item 5 is unblocked. New `dxcluster/dxcc_table`: parser for AD1C `cty.dat` (alias markup stripped, exact `=CALL` entries, west-positive longitude flipped, entities only committed once their alias list ends with `;`) and `lookup()` (longest prefix; portable forms EA8/K1ABC, K1ABC/EA8, /P, /3; /MM and /AM return nothing). The snapshot (MIT licence, notice in docs/third-party-cty.md, recorded in the roadmap's data source policy) ships as a Qt resource and is parsed on first use. DX cluster rows gain a `country` role (no QML change yet, so no screenshots). Tests run on a synthetic file and on the real snapshot; normal and ASan/UBSan runs pass. The row role itself is not unit-tested (resources are only linked into the app).
- Not done: country in the card/map labels, DX spots on the map (need a prefix-centre policy), DXCC-entity watching, NADXCC keywords, HamAlert map overlay. Not validated on device.
- Next: use the table for DX cluster/HamAlert map markers and DXCC watch entries, then ADIF not-in-log keywords.

## 2026-10-14 (run 13)
- Step 1: nothing of mine open; #196 (CI running) and #73 are other areas', left alone. No open `[C]` qa issues.
- Backlog item 9 (slice): new `src/widgets/aprs/`: `parse_packet` (plain and compressed position reports, optional timestamp, ambiguity spaces as zero; Mic-E, objects, messages ignored), `AprsModel` (APRS-IS rotate.aprs2.net:14580, receive-only login `pass -1` with a 100 km server filter around DE, nothing else sent, backoff reconnect, 30 min age-out, 200 cap, Not configured while the callsign is empty), compact card with nearest stations. No AppSettings change. Tests (fake socket) pass normally and under ASan/UBSan (leak check off).
- Not verified: port 14580 is not reachable from the sandbox, so the live login reply is untested. Card not on the default dashboard, so no screenshots; not added to the pane chooser (area A's file); not validated on device.
- Not done: radius/server settings, map overlay, Mic-E decoding, Meshtastic, band-activity heatmap.
- Next: APRS map overlay, HamAlert map overlay, OTA filter controls in Settings, DXCC use in labels and watch entries.

## 2026-10-14 (run 14)
- Step 1: nothing of mine open; #205 (qa log) has a failing iPad Simulator check and #73 is stale, both other areas', left alone. No open `[C]` qa issues.
- Backlog item 11 (slice): new `bandactivity/band_activity.hpp` (`summarize`: per-band spot totals split by continent, in band order, resolver injected). `DxSpotList::activity()` uses it with the bundled DXCC table; `DxClusterModel.activity` property feeds a "Busiest 20m · 12 (EU 7 NA 5)" line in the DX cluster card (continent split hidden in compact mode). No AppSettings change. Tests pass normally and under ASan/UBSan (leak check off).
- Not done: standalone band × continent heatmap pane, PSKReporter/WSPR/RBN as sources, screenshots (cluster stays Not configured without a callsign, so the line cannot be rendered here). Not validated on device.
- Next: APRS/HamAlert map overlays, OTA filter controls in Settings, DXCC country in card/labels, Meshtastic.

## 2026-10-14 (run 15)
- Step 1: nothing of mine open; #207 (no checks reported), #213 (macOS universal running), #73 are other areas', left alone. No open `[C]` qa issues.
- Slice of backlog item 5: the DX cluster card now shows the DXCC country on each row (`country · comment`; country only in compact mode). QML-only change, existing tests pass (127/127). No screenshots: the cluster stays Not configured without a host/callsign, so rows cannot be rendered offline. Not validated on device; ASan/UBSan left to CI for this QML-only change.
- Next: country in map labels, APRS/HamAlert map overlays, OTA filter controls in Settings, Meshtastic.

## 2026-10-14 (run 16)
- Step 1: nothing of mine open; #73 (docs, other area) left alone. No open `[C]` qa issues.
- Backlog item 1 (slice): DX cluster spots on the map via the overlay API. New `dxcluster/dx_overlay`: `overlay_layer` draws one marker per DXCC entity at the cty.dat entity centre (spots carry no position), newest first, capped at 40, prefix labels, dashed path from DE. `DxClusterModel` publishes the layer only while the link is live and removes it otherwise; `setMapVisible` is the per-source hook (Settings toggle not wired). No AppSettings change. Tests (synthetic resolver; model publish/remove) pass normally and under ASan/UBSan for the two touched test binaries.
- Not done: Settings toggle, band colouring, HamAlert/APRS overlays. Entity centres are approximate (not the DX station's location). The cluster stays Not configured without a callsign, so no screenshot of the layer; not validated on device.
- Next: APRS/HamAlert overlays, OTA filter controls in Settings, DXCC-entity watching, Meshtastic.

## 2026-10-14 (run 17)
- Step 1: nothing of mine open; #220 (lookup, no checks reported) and #73 (stale docs) are other areas', left alone. No open `[C]` qa issues.
- Backlog item 14 (slice): APRS stations on the map via the overlay API. New `aprs/aprs_overlay` (`overlay_layer`: diamond markers, call labels, comment as tap detail, capped at 60, non-finite positions skipped); `AprsModel` publishes the layer only while the link is live (most recently heard first) and removes it on loss/stop/destruction; `setMapVisible` is the per-source hook (Settings toggle not wired). No AppSettings change. Tests pass normally (132/132) and the aprs test under ASan/UBSan (leak check off).
- Not done: Settings toggle, HamAlert overlay, radius setting. APRS needs a callsign so no screenshot of the layer; not validated on device.
- Next: HamAlert overlay, OTA filter controls in Settings, DXCC-entity watching, Meshtastic.

## 2026-10-14 (run 18)
- Step 1: merged #230 (qa log) and #231 (ui units), both green; #73 is stale docs, left alone. Nothing of mine open. No open `[C]` qa issues.
- Backlog item 14 (slice): HamAlert alerts on the map. `HamAlertModel` publishes layer `hamalert` (reuses the DX overlay builder: one marker per DXCC entity centre, newest first, own colour) only while the feed is live, removed on loss/stop/destruction; `setMapVisible` is the per-source hook (Settings toggle not wired). No AppSettings change. Test uses a synthetic resolver.
- Not done: Settings toggle, trigger names, ONTA/ADIF overlays. Needs HamAlert login so no screenshot; not validated on device.
- Next: OTA filter controls in Settings, DXCC-entity watching, Meshtastic, ONTA/ADIF overlays.

## 2026-10-14 (run 19)
- Step 1: merged #237 (ui callsign banner, green); #238 (CI running) and #73 are other areas', left alone. Nothing of mine open. No open `[C]` qa issues.
- Backlog item 16 (slice): DX cluster rows gain an `ota` role ("POTA K-1234", from the already-parsed comment reference) and the card shows it as an accent badge before the country/comment. No AppSettings change. Test added; 132/132 pass. ASan/UBSan left to CI (small model/QML change); no screenshot since the cluster stays Not configured without a callsign. Not validated on device.
- Not done: IOTA reference names, ONTA/ADIF overlays, OTA filter controls in Settings, DXCC-entity watching, Meshtastic.
- Next: OTA filter controls in Settings, DXCC-entity watching, Meshtastic.

## 2026-10-14 (run 20)
- Step 1: nothing of mine open; #242 (no checks reported) and #73 are other areas', left alone. Open `[A]` qa issues only, no `[C]`.
- Backlog item 4 (slice): DXCC-entity watching. Watch entries of the form `@PREFIX` (cty.dat primary prefix, e.g. `@VP8`) match every call resolving to that entity; `watchlist::matches` takes an optional entity resolver, `DxSpotList::set_entity_resolver` injects one (default: bundled table). Settings hint text updated. No AppSettings field change (same `watch/list` key; older lists unaffected). Tests pass normally and under ASan/UBSan (leak check off).
- Not done: entity names (instead of prefixes) in entries, watching on HamAlert/OTA cards. No screenshot (cluster Not configured without a callsign). Not validated on device.
- Next: OTA filter controls in Settings, Meshtastic, ONTA/ADIF overlays.

## 2026-10-14 (run 21)
- Step 1: nothing of mine open; #255 (ui, macOS checks queued) and #73 (stale docs) are other areas', left alone. Open `[A]`/`[D]`/`[G]` qa issues only, no `[C]`.
- Backlog item 15 (slice): NADXCC/NABand queries. `AdifModel::entityNeeded(call)` and `bandSlotNeeded(call, band)` use a DXCC resolver (bundled cty.dat table by default, injectable) over the loaded log; both return false without data or for unknown entities. No AppSettings change. Test with a synthetic resolver passes normally and under ASan/UBSan (leak check off).
- Not done: wiring into DX cluster/HamAlert rows and watch keywords (NADXCC etc.), NAPref/NAGrid, ADIF map overlay. Not validated on device.
- Next: wire "needed" badge into the DX cluster card, OTA filter controls in Settings, Meshtastic.

## 2026-10-14 (run 22)
- Step 1: nothing of mine open; #264 (ui) and #73 (stale docs) are other areas', left alone. Open `[A]`/`[D]`/`[G]` qa issues only, no `[C]`.
- Backlog item 15 (slice): "needed" badge on DX cluster rows. `DxSpotList` has a `needed` role (NEW DXCC / NEW BAND) from an injectable resolver; the registered widget wires it to the ADIF model's `entityNeeded`/`bandSlotNeeded` via the registry by id at run time (no link dependency; no badge without an ADIF log). No AppSettings change. Test with a fake resolver passes normally and under ASan/UBSan (leak check off).
- Not done: NADXCC/NABand watch keywords, NAPref/NAGrid, ADIF map overlay. No screenshot (cluster Not configured without a callsign). Not validated on device.
- Next: OTA filter controls in Settings, Meshtastic, ONTA/ADIF overlays.

## 2026-10-14 (run 23)
- Step 1: merged #271 (docs, green, clean). #73 conflicts (dirty) and #273 had no checks reported yet; both other areas', left alone. Nothing of mine open. No open `[C]` qa issues.
- Backlog item 7 (slice): OTA filter controls in Settings. `AppSettings` gains `otaProgram/otaBand/otaMode/otaMaxAgeMin` (options.ini `[ota]`, validated, defaults = no restriction, backward compatible) and `saveOtaFilter`; `OtaModel` follows them via `optionsChanged`. Settings Spots card has four fields saved with "Save cluster". Test covers validation, live model update, restart persistence. 138/138 pass; ASan/UBSan left to CI. No screenshot of the Settings page; not validated on device.
- Not done: WWFF/IOTA/GMA etc. programmes, per-source map toggle in Settings, sort option.
- Next: Meshtastic, ONTA/ADIF overlays, per-source map toggles.

## 2026-10-14 (run 24)
- Step 1: nothing of mine open; #279/#280 (other areas) had checks still running and #73 is stale docs, all left alone. Open `[A]`/`[D]`/`[F]`/`[G]` qa issues only, no `[C]`.
- Settings parity (APRS): `AppSettings` gains `aprsHost/aprsPort/aprsRadiusKm` (options.ini `[aprs]`, validated, defaults rotate.aprs2.net / 14580 / 100, backward compatible), `saveAprs` and an `aprsChanged` signal emitted only on a real change. `AprsModel` uses them for connection and the `r/` filter and reconnects on change. Settings Spots card has three fields saved with "Save cluster". Per-source map toggles already exist in area G's Map menu (layer visibility), so no separate toggle was added. Test covers validation, reconnect, filter radius, no-op save, persistence; 138/138 pass. ASan/UBSan left to CI; no Settings screenshot; not validated on device.
- Not done: Meshtastic, ONTA/ADIF overlays, WWFF/IOTA/GMA programmes.
- Next: Meshtastic, ONTA/ADIF overlays.

## 2026-10-14 (run 25)
- Step 1: merged #286 (apple, all green). #287 had macOS checks still running and #73 is stale docs, both other areas', left alone. Nothing of mine open. No open `[C]` qa issues.
- Backlog item 15 (slice): ADIF map overlay. Layer `adif` (square markers, one per distinct base call, newest QSO wins, capped at 200) placed at the centre of the logged Maidenhead grid (2/4/6 chars); QSOs without a valid grid are not drawn. Published only while a log is loaded, removed on clear/error/destruction; `setMapVisible` is the per-source hook. No AppSettings change. Offline test added.
- Not done: Meshtastic (needs MQTT/protobuf or a node link; source decision pending), ONTA overlay, NADXCC/NAPref/NAGrid watch keywords. No screenshot (needs a user log); not validated on device.
- Next: Meshtastic, ONTA overlay, NAGrid.

## 2026-10-14 (run 26)
- Step 1: nothing of mine open; #294/#295/#296 (other areas) had checks still running and #73 is stale docs, all left alone. Open `[A]`/`[D]`/`[F]`/`[G]` qa issues only, no `[C]`.
- Backlog item 15 (slice): `NADXCC` / `NABAND` watch-list keywords. They highlight (★, watched count) DX cluster spots the ADIF log still needs (NADXCC: new entity; NABAND: new entity or band slot); re-evaluated when the resolver or list changes. No AppSettings change (same `watch/list` key; the Settings hint mentions the keywords). Test added; 142/142 pass; ASan/UBSan left to CI (small model change). No screenshot (cluster Not configured without a callsign). Not validated on device.
- Not done: NAPref/NAGrid, ONTA overlay, Meshtastic (source decision pending).
- Next: Meshtastic, ONTA overlay, NAGrid.

## 2026-10-14 (run 27)
- Step 1: nothing of mine open; #304 (other area, checks not yet reported) and #73 (stale docs) left alone. No open `[C]` qa issues.
- Backlog item 7 (slice): OTA sort option. `ota::Filter` gains `sort` (time default / freq ascending / dist nearest to DE first, spots without a position last) and DE position; `AppSettings` gains `otaSort` (options.ini `[ota] sort`, validated, default newest first, backward compatible) and `saveOtaSort`; the model follows settings and station changes. Settings Spots card has a Sort field. Test covers ordering, validation, persistence. 142/142 pass; ASan/UBSan left to CI. No Settings screenshot; not validated on device.
- WWFF (spots.wwff.co) and GMA (cqgma.org) are not reachable from the sandbox; added to the allowlist issue.
- Next: WWFF/GMA once allowlisted, Meshtastic, ONTA overlay, NAGrid.

## 2026-10-14 (run 28)
- Step 1: nothing of mine open; #310 (ui, macOS/tvOS checks still running) and #73 (stale docs) are other areas', left alone. No open `[C]` qa issues.
- Review: backlog item 14 (label modes, minimum label distance) is already provided by area G's map label mode and per-layer settings; NAGrid needs a grid per spot, which cluster spots lack, so it is not pursued. ONTA overlay already exists.
- Blocked: Meshtastic (C10) needs a source decision; opened "Owner input needed: Meshtastic source" (#311) with options. No code change this run.
- Next: UDP spot input (item 12), WSPR/RBN once a source is chosen, WWFF/GMA once allowlisted (#305).

## 2026-10-15 (run 29)
- Step 1: nothing of mine open; #319 (bands) had checks running and #73 is stale docs, both other areas', left alone. No open `[C]` qa issues.
- Backlog item 12 (slice): WSJT-X binary UDP protocol (schema 2/3) on the existing opt-in UDP port. `WsjtxDecoder` keeps the dial frequency per client id from Status messages and turns Decode messages "CQ [DX] CALL [GRID]" into spots (dial + audio offset, FT8/FT4 mode, spotter "WSJT-X"). Bounds-checked reader; wrong magic, truncated, oversized, non-CQ and decodes before a Status are ignored. No AppSettings change, no new listener. Test builds datagrams by hand; udp/dx tests pass, full suite run locally; ASan/UBSan left to CI. Not validated on device.
- Not done: multicast, "UDP sets DX", by me/all filter, Log4OM format, WSJT-X Logged/QSO messages.
- Next: UDP sets DX / by-me filter, WSPR/RBN once a source is chosen, WWFF/GMA once allowlisted (#305).

## 2026-10-15 (run 30)
- Step 1: merged #327 (qa log, Qt checks green). #73 is stale docs, another area's, left alone. No open `[C]` qa issues.
- Backlog item 12 (slice): UDP "by me"/"all" scope. `udp_spot_in_scope` keeps spots from local programs (spotter UDP/WSJT-X) or whose spotter has my base call (SSID, /P ignored); "all" keeps everything (default). `AppSettings` gains `udpSpotByMe` (options.ini `[udp] scope=all|me`, backward compatible) and `saveUdpScope`; Settings Spots card has a toggle saved with "Save cluster". Test covers both scopes. 145/145 pass locally; ASan/UBSan left to CI. No Settings screenshot; not validated on device.
- Not done: multicast, "UDP sets DX", Log4OM format.
- Next: UDP sets DX, WSPR/RBN once a source is chosen, WWFF/GMA once allowlisted (#305).

## 2026-10-15 (run 31)
- Step 1: nothing of mine open; #338 (space, checks running) and #73 (stale docs) are other areas', left alone. No open `[C]` qa issues.
- Backlog item 6 (slice): PSKReporter band statistics. `psk::band_stats/band_summary` count reports per amateur band (busiest first, out-of-band skipped); `PskModel::bands` and a "20m 5 · 40m 2" line (max four bands) on the card. No AppSettings change, no new source. Test added; 147/147 pass locally, zero warnings; ASan/UBSan left to CI (small pure functions). No screenshot (card needs a callsign); not validated on device.
- Not done: WSPR/RBN sources (RBN is telnet, source decision pending), by-call/grid and max-distance filters.
- Next: UDP sets DX (#333), Meshtastic (#311), WWFF/GMA (#305) all wait on owner input.

## 2026-10-15 (run 32)
- Step 1: merged #343 (ui alarms, all checks green). #73 is stale docs, left alone. Nothing of mine open; no open `[C]` qa issues.
- Backlog item 6 (slice): PSKReporter "Farthest: CALL km band" line (mi when imperial units are set), from the receiver grid centre to DE. `psk::farthest_report` is pure; the card line hides when no report has a usable grid. No AppSettings change, no new source. Test added; 148/148 pass locally; ASan/UBSan left to CI. No screenshot (card needs a callsign); not validated on device.
- Not done: WSPR/RBN, by-call/grid and max-distance filters. Meshtastic (#311), UDP sets DX (#333), WWFF/GMA (#305) still wait on owner input.

## 2026-10-15 (run 33)
- Step 1: nothing of mine open; #348 (ui) had checks still running and #73 (stale docs) is another area's, both left alone. No open `[C]` qa issues.
- Backlog item 6 (slice): PSKReporter mode statistics. `psk::mode_stats` counts reports per mode (case folded, busiest first, empty skipped); `PskModel::modes` and a "FT8 6 · CW 2" line on the card (hidden in compact layout). No AppSettings change, no new source. Test added. Not validated on device; no screenshot (card needs a callsign).
- Not done: WSPR/RBN, by-call/grid and max-distance filters. Meshtastic (#311), UDP sets DX (#333), WWFF/GMA (#305) still wait on owner input.

## 2026-10-15 (run 34)
- Step 1: nothing of mine open; #73 (stale docs) is another area's, left alone. No open `[C]` qa issues.
- Backlog item 15 (slice): `NAPREF` watch keyword and NEW PREFIX badge. `adif::prefix_of` (letters+first digit run, `EA8/` operating prefix wins), `AdifModel::prefixNeeded` (false without a loaded log). Badge priority NEW DXCC > NEW PREFIX > NEW BAND; NAPREF matches NEW PREFIX only, NABAND any needed. No AppSettings change (Settings hint text mentions NAPREF). Tests added; 151/151 pass locally, no warnings in touched files; ASan/UBSan left to CI. Not validated on device.
- Not done: NAGrid (cluster spots carry no grid).

## 2026-10-15 (run 35)
- Step 1: nothing of mine open; #360 (ui, macOS/iPad/tvOS checks running) and #73 (stale docs) are other areas', left alone. No open `[C]` qa issues.
- Backlog item 6 (slice): PSKReporter max-distance filter. `psk::within_distance` (grid centre vs DE; with a limit, reports lacking a usable grid are dropped), `PskModel` keeps the raw feed and re-filters on settings change; list, stats, farthest and map all use the filtered set. `AppSettings` gains `pskMaxKm` (options.ini `[psk] max_km`, 0 = any, backward compatible) and `savePskMaxKm`; Settings Spots card field saved with "Save cluster". Tests added; 151/151 pass locally. No new warnings in touched files; ASan/UBSan left to CI. No screenshot (card needs a callsign; one extra Settings field); not validated on device.
- Not done: PSK by-call/grid filter, WSPR/RBN. Meshtastic (#311), UDP sets DX (#333), WWFF/GMA (#305) still wait on owner input.

## 2026-10-15 (run 36)
- Step 1: nothing of mine open; #73 (stale docs) is another area's, left alone. No open `[C]` qa issues.
- Backlog item 14 (slice): per-source map on/off wired to Settings. `AppSettings::mapSourceVisible/saveMapSourceVisible` (options.ini `[map_sources] hidden=`, default all on, backward compatible); the DX, PSK, POTA, HamAlert, APRS and ADIF models read it at start and follow `optionsChanged` via their existing `setMapVisible`. Settings Spots card gets a "Show on map" row of toggle buttons (applies immediately). Test in the PSK regression; 151/151 pass locally. QML loads without warnings at 800x480 and 1024x600, but the Settings page is not reachable via `--screenshot`, so no Settings screenshot; ASan/UBSan left to CI. Not validated on device.
- Not done: label styles (None/Prefix/Call/Dot), minimum label distance.

## 2026-10-15 (run 37)
- Step 1: #370 (space) had macOS/iPad checks still running and #73 is stale docs; both other areas', left alone. Nothing of mine open; no open `[C]` qa issues.
- Backlog item 6 (slice): PSKReporter by-call/grid filter. `psk::match_tokens` (normalise, max 8 tokens) and `psk::matching` (receiver call or grid starts with a token); `AppSettings::pskMatch/savePskMatch` (options.ini `[psk] match`, empty = all, backward compatible); Settings Spots card field saved with "Save cluster". Applies to list, stats, farthest and map. Tests extended in the PSK regression; 151/151 pass locally. No screenshot (card needs a callsign; one extra Settings field); ASan/UBSan left to CI; not validated on device.
- Not done: WSPR/RBN. Meshtastic (#311), UDP sets DX (#333), WWFF/GMA (#305) still wait on owner input.

## 2026-10-15 (run 38)
- Step 1: nothing of mine open; #73 (stale docs) is another area's, left alone. No open `[C]` qa issues.
- Backlog item 14 (slice): per-source label style and minimum label distance. `AppSettings::mapSourceLabel/saveMapSourceLabel` (options.ini `[map_sources] label_<src>` = 0 None, 1 Prefix, 2 Call, 3 Dot; absent = layer's own) and `mapMinLabelPx/saveMapMinLabelPx` (`min_label_px`, 0 = layer's own); backward compatible. `apply_spot_label_style` (dxcluster/spot_layer_style.hpp) is applied by the DX, PSK, POTA, HamAlert, APRS and ADIF models, which now republish on any settings change. Settings Spots card gets a "Map labels" button row. Test in the PSK regression; 151/151 pass locally; ASan/UBSan left to CI. Settings page not reachable via `--screenshot`, so no screenshot; not validated on device.
- Not done: WSPR/RBN. Meshtastic (#311), UDP sets DX (#333), WWFF/GMA (#305) still wait on owner input.

## 2026-10-15 (run 39)
- Step 1: nothing of mine open; #73 (stale docs) is another area's, left alone. No open `[C]` qa issues.
- Backlog item 8 (slice): HamAlert rows now show the xOTA reference parsed from the alert comment (`ota` field, "POTA K-1234") on the card. The alert/trigger name is not parsed: its wire format is not documented or fixtured, so nothing is invented. No AppSettings change. Test extended; 152/152 pass locally, no warnings in touched files; ASan/UBSan left to CI. No screenshot (card needs a login); not validated on device.
- Not done: HamAlert trigger names/per-alert filters, WSPR/RBN. Meshtastic (#311), UDP sets DX (#333), WWFF/GMA (#305) wait on owner input.

## 2026-10-15 (run 40)
- Step 1: nothing of mine open; #73 (stale docs) is another area's, left alone. No open `[C]` qa issues.
- Backlog item 6 (slice): WSPR parser only. `psk::parse_wspr` (wspr.live JSON, frequency in Hz, mode "WSPR", reuses `psk::Report`) and `psk::wspr_query_url` (by DE, last hour, LIMIT 100). Fixture `wspr-rx.json` is a trimmed real capture (Oct 15). wspr.live is reachable from the sandbox; terms recorded in the roadmap's Data source policy (fair use, owner to confirm). Nothing fetches it yet, so no UI change or screenshot. 152/152 pass locally; ASan/UBSan left to CI. Not validated on device.
- Next: WSPR fetch in `PskModel` (5 min poll, source toggle), RBN (telnet, source decision pending).

## 2026-10-15 (run 41)
- Step 1: #383 (apple, no checks reported) and #73 (stale docs) are other areas', left alone. Nothing of mine open; no open `[C]` qa issues.
- Backlog item 6 (slice): WSPR fetch. `PskModel` runs a second `PolledSource` on `psk::wspr_query_url` (5 min) and merges the spots into the PSKReporter list, stats, farthest and map; all existing filters apply. Opt-in: `AppSettings::pskWspr/savePskWspr` (options.ini `[psk] wspr`, default off, backward compatible) with a Settings button, because the wspr.live fair-use terms still need owner confirmation. Card state stays driven by PSKReporter. Test added with the real fixture. Not validated on device; no screenshot (card needs a callsign).
- Not done: RBN, WSPR "of DE" direction.

## 2026-10-15 (run 42)
- Step 1: merged #388 (ui, all checks green). #389 (apple) still running and #73 (stale docs) are other areas', left alone. Nothing of mine open; no open `[C]` qa issues. Could not delete #388's head branch (denied by the sandbox classifier); left for the owner.
- Backlog item 6 (slice): WSPR "of DE" direction. `psk::wspr_heard_url` (rx_sign = DE), `psk::heard_as_reports` (swap so distance helpers apply), second wspr.live source in `PskModel` (same opt-in, same 5 min poll), card line "I heard N · farthest CALL km band". Not mixed into the "hears me" list or map. No AppSettings change. Tests extended; 153/153 pass locally, no warnings; ASan/UBSan left to CI. No screenshot (card needs a callsign); not validated on device.
- Not done: RBN, WSPR by-call queries, map paths for heard stations.

## 2026-10-15 (run 43)
- Step 1: #392 (ui) still has macOS/iPad/ASan checks running and #73 is stale docs; both other areas', left alone. Nothing of mine open; no open `[C]` qa issues.
- Backlog item 15 (slice): NAGRID watch keyword. `AdifModel::gridNeeded` (valid 4-char locator not in the log; false when the log is not loaded), `DxSpotList::set_grid_needed_resolver`, wired by the DX cluster registration like the other needed resolvers. Only spots that carry a locator in the line can match (nothing is invented); no AppSettings change; Settings hint mentions NAGRID. Tests extended; 154/154 pass locally; ASan/UBSan left to CI. No screenshot (text hint only); not validated on device.
- Not done: WSPR/RBN by-call, IOTA names. Meshtastic (#311), UDP sets DX (#333), WWFF/GMA (#305) wait on owner input.

## 2026-10-15 (run 44)
- Step 1: #403 (ui) still has checks running and #73 is stale docs; both other areas', left alone. Nothing of mine open; no open `[C]` qa issues.
- Backlog item 6 (slice): map paths for stations DE heard on WSPR. `psk::heard_overlay_layer` (layer `psk-heard`, amber markers, path from DE) published by `PskModel` only while the opt-in WSPR "heard" feed is fresh and the PSK map toggle is on; removed otherwise. Shares the `psk` label style. No AppSettings change. Test extended; 154/154 pass locally; ASan/UBSan left to CI. No screenshot (needs a callsign and live data); not validated on device.
- Not done: RBN (#399 owner input), WSPR by-call queries.

## 2026-10-15 (run 45)
- Step 1: nothing of mine open; #73 (stale docs) is another area's, left alone. No open `[C]` qa issues.
- Backlog item 11 (slice): standalone band-activity heatmap pane. New `bandactivity` widget (`BandActivityModel`, `BandActivityCard.qml`) shows band × continent counts from the DX cluster model's `activity`, found through the registry at run time (no link dependency). It mirrors the cluster's state: unconfigured/loading/stale/offline are never shown as live, and a live link with no spots is "loading". Compact card shows the busiest band only. Pane chooser category added (2). No AppSettings change. Test with a fake source added; 156/156 pass locally. No screenshot (needs a live cluster link, not available offline); ASan/UBSan left to CI; not validated on device.
- Not done: PSK/WSPR/RBN as heatmap sources, WSPR by-call queries, IOTA names.

## 2026-10-15 (run 46)
- Step 1: #73 (stale docs) is another area's, left alone. Nothing of mine open; no open `[C]` qa issues.
- Backlog item 12 (slice): UDP multicast. `UdpSpotListener::listen(port, lan, group)` binds shared on all IPv4 interfaces and joins an IPv4 multicast group (e.g. WSJT-X's 224.0.0.1); invalid or non-multicast groups are refused. `AppSettings::udpSpotGroup/saveUdpGroup` (`[udp] group`, empty = unicast, backward compatible) and a Settings field. `DxClusterModel::refreshUdp` now also rebinds when the LAN flag or group changes. Tests extended (join only asserted where the sandbox allows it); UDP/DX tests pass locally, full ctest run exceeded the tool timeout here, ASan/UBSan left to CI. No screenshot (Settings not reachable via `--screenshot`); not validated on device.
- Not done: "UDP sets DX" (#333), Log4OM, RBN (#399).

## 2026-10-15 (run 47)
- Step 1: #411 (ui) and #73 (stale docs) are other areas', left alone. Nothing of mine open; no open `[C]` qa issues.
- Docs-only slice: the DX cluster, PSKReporter and HamAlert rows still said the map Settings toggle was not wired, but `SettingsPage.qml` ("Show on map": DX, PSK, POTA, HamAlert, APRS, Log) and the models' `mapSourceVisible` hooks already exist. Rows corrected; no code change, no tests affected.
- Not done: RBN (#399), WSPR by-call, IOTA names, other ONTA programmes (#305). Meshtastic (#311), UDP sets DX (#333) wait on owner input.

## 2026-10-16 (run 48)
- Step 1: only #73 (stale docs, another area's) open; left alone. No open `[C]` qa issues.
- Backlog item 15/16 (slice): "On The Air" watch keywords. `OTA` (any reference) and `POTA`/`SOTA`/`IOTA`/`WWFF` in the watch list highlight DX cluster spots whose comment names such a reference (from the existing parser; nothing invented). No AppSettings change; Settings hint updated. Test extended; dx/watch tests pass locally, full ASan/UBSan left to CI. No screenshot (text hint only); not validated on device.
- Not done: RBN (#399), WSPR by-call, IOTA names. Meshtastic (#311), UDP sets DX (#333), WWFF/GMA (#305) wait on owner input.

## 2026-10-16 (run 49)
- Step 1: only #73 (stale docs, another area's) open; left alone. No open `[C]` qa issues.
- Remaining C items are blocked (RBN #399, Meshtastic #311, UDP sets DX #333, WWFF/GMA #305, IOTA names #426) or need a real HamAlert/ONTA fixture that does not exist, so nothing invented. Docs-only slice: backlog item 9 still said APRS settings and map overlay were "later"; both are done (see settings parity row). No code change.

## 2026-10-17 (run 50)
- Step 1: only #73 (stale docs, another area's) open; left alone. No open `[C]` qa issues.
- Found that #399 (RBN), #333 (UDP sets DX) and #311 (Meshtastic) already carry owner decisions from Oct 15; earlier runs logged them as waiting. Next runs: #333 and #311 are unblocked.
- Backlog item 6 (slice): RBN "by DE". `psk::parse_rbn_line` (reuses the DX parser; skimmer `-#` dropped, mode and dB from the comment), `PskModel` opens one receive-only telnet connection to telnet.reversebeacon.net:7000 only when the new opt-in `AppSettings::pskRbn/savePskRbn` (`[psk] rbn`, default off, backward compatible) is on and a callsign is set; reports kept 15 min, max 200, merged into the PSKReporter card. Settings button added. Tests extended (fake socket, inline lines because raw TCP is unreachable from the sandbox, so no real capture); 156/156 pass locally; ASan/UBSan left to CI. No screenshot (card needs a callsign and a live link); not validated on device.
- Not done: RBN port 7001 (FT8/FT4, would be a second connection), UDP sets DX (#333), Meshtastic (#311).

## 2026-10-17 (run 51)
- Step 1: only #73 (stale docs, another area's) open; left alone. No open `[C]` qa issues.
- Backlog item 12 (slice): "UDP sets DX" per the owner decision on #333 (option 1). New opt-in `AppSettings::udpSetsDx/saveUdpSetsDx` (`[udp] sets_dx`, default off, backward compatible) and a Settings button. Shared-file change (`AppSettings`, minimal): `setDxOverride/clearDxOverride/dxOverrideActive`; the override replaces the effective DX in `station_` but is never written to the station file (`persistable()`), and an explicit DX save replaces it. `DxClusterModel::applyDxOverride`: only spots from this station's own programs (WSJT-X/UDP, no foreign spotter) carrying a 4/6-char locator (`locator_centre`), 10 min expiry restarted by each such spot, cleared when the option is turned off. New test `dx_override_regression`; 157/157 pass locally; ASan/UBSan left to CI. No screenshot (Settings button only); not validated on device.
- Not done: Meshtastic (#311, unblocked), Log4OM, RBN port 7001.
