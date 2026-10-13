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
