# Independent native shell audit

Status: **frozen shell source and executable integration checks pass; root native visual comparison pending**. Reviewed 2026-10-03. Reviewer owns only this report and has changed no production source, tests, build files or Git state.

## Frozen map scope

The final inspected diff changes the dark ocean from `#24383c` to `#374a51`, dark coastline from `#9fb5b7` to `#8b9fa3`, and fills the entire widget background with its active ocean colour rather than its inherited Window palette. That last correction keeps letterboxing in the selected theme; geographic clipping/aspect bounds remain 2:1. Light ocean/coastline and both route colours are unchanged. Two pixel assertions now expect the changed dark ocean and a new outside-map pixel assertion checks dark letterboxing. The official coastline asset, projection, part/seam handling, route, endpoints and labels are unchanged. Coastlines remain unfilled QPainter paths with `Qt::NoBrush`; there is no invented land geometry or solid land fill.

An independent rebuild in a separate temporary CMake directory and the map CTest passed. Existing tests still check rendered coastline presence within broad pixel-area bounds, explicit themes, live System palette changes, endpoint validity, routes and small sizes. Pixel presence proves drawing occurred; it does not prove users can comfortably distinguish the softened geography.

**Readability finding repaired:** the initial softened coastline proposal measured approximately 1.37:1 Light and 2.38:1 Dark. After the reviewer reported that visibility concern, root restored Light coast `#647c7e` against `#dce8eb` and increased Dark coast to `#8b9fa3` against `#374a51`. Independent recalculation confirms approximately **3.55:1 Light** and **3.36:1 Dark**. The separately rebuilt map CTest passed again after the correction. This intentional readability adjustment differs from the initial mock tokens. Anti-aliased edge pixels can still be closer to the ocean colour; nominal colour contrast and passing pixel tests are not accessibility conformance or legal clearance.

## Dashboard source and regression review

Final author/root freeze was confirmed before this verdict. Reviewed the grouped station/sidebar, instrument strip, map card, path action, compact/large adaptation and separate dashboard appearance helper. Instrument and cluster panes state **Unavailable** or **Not connected**; they contain no fabricated observations, solar imagery or propagation result. Callsign, coordinates, grids, distance, bearing and map description derive from the current station snapshot. Clock/date derive from operating-system UTC. Existing accessible names retain the actual current values after both clock and Save updates.

Appearance selection is explicitly an unsaved preview: the control's accessible description tells users to save the preference in station settings. Preview updates shell/map presentation without changing the stored station snapshot or disk. A successful editor Save refreshes snapshot, labels, map and selected theme; invalid Save, filesystem failure and Cancel preserve earlier state. The edit action restores focus after its modal closes. The route action updates its text and accessible map description without altering persisted station settings.

The appearance observer is dialog-owned with a guarded dialog pointer; its deferred refresh uses itself as the timer context. A mode check at both scheduling and execution prevents a queued System update overriding a newly chosen explicit theme. The pending flag coalesces application-palette notifications. Repeated application creates one direct-child observer. No external I/O or additional dependency was introduced by presentation helpers.

Fresh independent ASan/UBSan builds passed **153 dashboard checks** and **158 map checks**, without sanitizer findings. The actual modal tests exercise Save/Cancel/invalid/file-error state against real temporary files and actual accessible interfaces. Compact 800×480 and large 1280×800 bounds, keyboard focus order, explicit/live System themes, rendered contrast samples and no map paints during clock ticks passed. A separate expanded temporary harness passed **157 dashboard checks**, adding invalid appearance without state mutation, observer uniqueness, an already queued System refresh followed by explicit Light, and safe destruction with a refresh pending.

Independent full `make verify` and a separate CMake build passed; all **seven CTest targets** passed. The new helper is linked in both dashboard application and test targets. These are bounded regression results, not guarantees about all screen sizes, OS themes or target hardware.

## Remaining visual and product gates

The following invariants were assessed for this increment, with actual root native visual verification still separate:

- Saved station values, actual UTC clock, coordinate/grid/distance/bearing and accessible names must stay synchronized; presentation must not introduce fabricated telemetry.
- Unimplemented data-provider panes must clearly show unavailable status rather than representative measurements presented as live data.
- Save must refresh the station snapshot, labels, map endpoints/route and theme only after persistence succeeds. Cancel, invalid edits and filesystem errors must preserve the earlier dashboard and disk state.
- Clock ticks must update the clock without regenerating geographic calculations or repainting the map unnecessarily; application/theme changes must still update appropriate visuals.
- Keyboard focus, controls, accessible text alternatives and readable theme colours need bounded executable checks plus root native visual verification. Explicit Light/Dark and live System changes must preserve logical focus and content.
- Appearance/layout tests must use current values and substantive assertions; screenshot comparisons are separate evidence and do not establish screen-reader, physical touch or Pi performance support.

No remaining source/integration blocker found in the frozen increment. Root must compare the actual native composition against the intended design before publication. Nominal contrast and offscreen geometry do not establish actual screen-reader announcements, physical touch usability, OS appearance transitions, minimum readable text for individual users, Pi performance or full HamClock parity. No accessibility compliance or product-release approval is granted by this audit.
