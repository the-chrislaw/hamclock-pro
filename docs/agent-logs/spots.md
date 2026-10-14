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
