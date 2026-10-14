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
