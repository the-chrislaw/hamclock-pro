# Agent log: qa

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (UTC) QA run 1
- Last reviewed commit: c63baa195b54e7c4b81a25c4db6fac89eafca55e (PRs #44 to #76, last 24 h).
- Build and 48/48 tests pass on main. Rendered 800x480, 1024x600 and 1920x1080, light and dark, with all widget fixtures; default panes only (see #80).
- Opened: #78 [B] Space weather X-ray and Solar wind Bz clipped; #79 [G] map letterboxed at wide sizes; #80 [A] no way to screenshot non-default panes. Closed: none.
- Code scan of lookup (#66) found no password exposure to QML or logs, no QtWidgets in widgets, no http:// URLs. Widget diffs were not reviewed line by line this run.
- Health: one-page layout works with no scrolling, but compact-pane clipping remains in the Space weather and Solar wind cards (already seen on the real Pi). The Bands, Lookup, Weather, Sun/Moon and Solar trend panes are not yet reviewable visually.

## 2026-10-14 (UTC) QA run 2
- Last reviewed commit: a695f6555546b0db1f3e77535d084afe1a90e972 (PRs #77 to #115, 40 merges since c63baa1).
- Build and 82/82 tests pass on main. Rendered every pane widget (watchlist is not a pane) at 800x480 and 1024x600 (light and dark), default panes at 1920x1080. Closed since last run: #78, #79, #80 (closed by their fixers; the clipped Space weather/Solar wind cards, map fill and `--pane` option are confirmed fixed on main).
- Opened: #117 [E] weather DE/DX columns overlap; #118 [D] Sun & Moon Moonset row sliced at 1024x600; #119 [A] card footer elides the data age; #120 [B] X-ray plot axis letters overprint at 1920x1080; #121 [B] Solar trend labels truncated at 800x480; #122 [G] map labels collide / do not scale; #123 [F] Linux credential store is plain JSON (0600).
- Diff scan of the 40 PRs: no QtWidgets in widgets, no http:// URLs, no scrolling views, no passwords in Q_PROPERTY or logs, no weakened test assertions. Per-widget code was not read line by line.
- Health: one-page layout is solid at all sizes and the earlier clipping is fixed; the remaining problems are small text-fit issues in newer widgets (weather, Sun & Moon, Solar trend) plus footer elision hiding data age. Suggest a CI check that renders every widget at 800x480 and fails on clipped text.
