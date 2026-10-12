# Next native dashboard integration slice

Prepared October 3, 2026 from repository source and the accepted `design/dashboard.html`. This is an implementation handoff, not a new mockup or a claim that the Qt dashboard exists.

## Concrete result

Build a separate dynamically linked Qt Widgets dashboard showing real system UTC, loaded DE/DX station coordinates and grids, computed distance/true bearing, and the licensed Natural Earth coastline with a computed short path. Use the accepted prototype's quieter layout, system type, opaque surfaces, restrained borders, and generous spacing. Open the native station editor from a standard button; successful Save refreshes all station-dependent widgets from one validated snapshot, while Cancel and failed Save leave the dashboard unchanged.

No illustrative propagation values, spots, solar picture, decorative routes, fake place names, or night shading should be carried into this runtime. Their reserved panes display readable “Not connected” or “Not implemented” states until real providers/calculations exist. This slice does not satisfy complete final-original HamClock parity.

## Evidence available now

- Accepted HTML uses a station sidebar, compact instrument strip, dominant map, System/Light/Dark control, larger-text and touch options. Its CSS defines light surfaces `#eeede9`/`#fafaf7`, ink `#303a42`, subdued blue accent `#487991`, and darker alternatives. Small prototype font sizes should not be copied blindly onto a seven-inch touch display.
- The native station editor currently uses standard Qt Core/Gui/Widgets controls, bounded `StationEdit`, explicit-path persistence, and create-only `save_new_settings`. Its source still has a standalone entry point, so dashboard invocation requires a small coordinated extraction rather than launching a subprocess and guessing whether Save occurred.
- `StationView` provides original coordinates, uppercase grids/callsign, rounded short-path kilometres and true bearing, including an undefined state. It is a data contract; Qt labels may use mixed-case surrounding text and real system fonts.
- `map_project_equirectangular`, inverse projection, seam splitting, and `map_short_path` are pure existing modules. The current geometry is equirectangular, not Mercator or a demonstrated set of original HamClock projections.
- `src/map/coastline.hpp` contains 5,128 vertices and 134 line parts from Natural Earth 110m version 4.1.0. `docs/map-provenance.md` records source hash, transformation, public-domain terms, and limits. These are coastlines, not verified land polygons.
- `editor_appearance` and its regression fixture establish theme-testing patterns. Existing editor documentation records a macOS explicit-Light defect and pending repaired-theme GUI validation. Recheck that gate rather than assuming the dashboard inherits a completed visual audit.

## Frozen interfaces to agree before coding

1. Dashboard consumes a complete `StationSettings` snapshot plus an explicit selected settings path. Existing-file launch errors are visible and never replaced with automatic defaults. An empty startup shell must mark station data unavailable until the user explicitly chooses a valid file or creates one.
2. Reusable dialog returns an explicit saved/cancelled outcome, and validated saved settings only after persistence succeeds. New destination uses `save_new_settings`; editing the selected existing file uses `save_settings`. Avoid treating dialog exit code zero as evidence of Save.
3. Map widget receives DE/DX, route availability, route-visibility flag, and theme colors; it performs no reads, writes, geolocation, provider calls, or radio control. Undefined antipodal routes omit the path but retain station markers and an explanatory summary.
4. Appearance has exactly `system`, `light`, and `dark`. System inherits the application/desktop palette and responds to actual palette changes. Explicit modes replace overrides deterministically. Changing a preview choice and cancelling the editor restores the dashboard's committed theme.
5. Clock display consumes UTC supplied by a clock adapter. UTC date and time use text widgets; synchronization quality must say “System clock” until a real health model exists. No local-time or DST result is invented.

## Exclusive file ownership for parallel implementation

These are proposed boundaries; the integrator must assign them before agents edit.

| Stream | New files owned | Responsibilities / forbidden shared changes |
|---|---|---|
| Dashboard implementer | `src/qt/dashboard_window.hpp`, `dashboard_window.cpp` | Semantic sidebar/header, pane placeholders, map host, readable summary, layout/reflow. Consumes contracts; does not edit settings/domain/map modules or build files. |
| Map implementer | `src/qt/map_widget.hpp`, `map_widget.cpp`, `tests/qt_map_render_regression.cpp` | Original QPainter coastlines/route/markers, seam splitting, clipping, resize/theme invalidation. No coastline asset edits, land-fill assumptions, UI dialogs, or network. |
| Appearance implementer | `src/qt/dashboard_appearance.hpp`, `dashboard_appearance.cpp`, `tests/qt_dashboard_appearance.cpp` | Soft surface tokens, standard-control styling, explicit/system theme transitions, focus contrast and text scaling. No editor styling file changes until integrator coordinates common extraction. |
| Integrator | `src/qt/dashboard_main.cpp`, build files; coordinated `station_dialog.hpp/.cpp` extraction and existing editor entry point | Launch modes, shared reusable dialog, Save outcome, snapshot updates, signal routing, target/module list. Retain editor executable and independent pure tests. |
| Independent auditor | `docs/qt-dashboard-audit.md` only | Reproduce build, inspect persistence transitions and failure handling, verify painted geometry and standard-widget semantics. File-specific findings return to implementers; auditor does not patch their files concurrently. |

With four available slots including the integrator, run three implementation streams, then freeze and hand them to an independent auditor. If dialog extraction is needed first, implementer stubs consume its agreed contract while the integrator handles the shared source. Do not assign two streams to `CMakeLists.txt`, the standalone editor, or existing appearance helper.

## Rendering and semantic requirements

Use native `QLabel`/buttons/comboboxes for callsign, DE/DX, coordinates, locators, UTC, distance, bearing, theme, settings entry, route visibility, and pane titles. Associate names and descriptions explicitly. Preserve keyboard focus when updating labels; focus must return to the settings-entry button after the dialog closes. Visible errors/statuses need words, not color alone. A custom painted map must have a concise description and equivalent readable standard-widget station/path summary; a description by itself does not make future map actions accessible.

Draw every coastline part independently using the existing seam splitter. Clip painter output to the map rectangle and preserve full-world aspect ratio. Cache projected coastline paths per geometry/size, recompute routes only when endpoints change, and repaint clock text independently of the map. Do not join distinct coastline parts, close open lines into invented land polygons, draw seam-spanning chords, or copy the prototype's approximate world paths. Filled land requires a separately verified polygon dataset/importer batch; coastlines alone can ship this bounded slice honestly.

Use a measured desktop-friendly base font and scalable layouts. At 800 × 480, prioritize UTC, DE/DX, map, and settings access; larger text may scroll/reflow rather than shrink labels to preserve a screenshot. Interactive touch controls target at least 44 logical pixels, subject to real screen scaling verification. Avoid blurs, continuous decorative animation, rasterized text, and full-window repaint timers.

## Meaningful regression and GUI gates

- Retain `make verify` pure/CLI/importer checks and station-model sanitizer tests. Add Qt build targets without changing the SDL experiment's source ownership.
- Render a known equatorial route and a date-line route; test marker placement and seam behavior against normalized projection fixtures, not a screenshot of the same implementation. Confirm antipodal route absence and visible undefined text.
- Test resize at 800 × 480 and a desktop size, with larger text and high display scaling. Check controls remain reachable, map has valid bounds, and labels do not overlap or silently truncate meaning.
- Exercise explicit Light on a dark application palette, explicit Dark on a light palette, repeated transitions, and returning to System. Check real rendered widget backgrounds/text plus palette-change handling; prior bug shows palette-only assertions are insufficient.
- With a real editor/dialog, load a station, change both endpoints, Save, and assert one consistent refreshed snapshot. Then edit/cancel, enter invalid coordinates, fail persistence, and attempt create-only publication over a newly appearing destination; old dashboard/configuration state must survive each failure.
- Keyboard Tab/Shift-Tab/Enter/Escape and route-toggle states must work without mouse. Inspect standard-widget names/roles and actual VoiceOver/Orca announcements, including errors. Qt metadata is groundwork, not ADA/WCAG certification.
- Capture the actual native dashboard, using nonpersonal fixtures, in both themes. Compare composition and softness to the accepted study; user review follows a working executable rather than another HTML approximation.

## Pi measurement and release risks

Pi 4 with the user's seven-inch touch display is the first real device test; Pi 3 remains required. Record OS/architecture, Qt version/plugins, X11/Wayland session, resolution/scaling, touch/input-method setup, storage, and power source. Measure cold and warm launch time, idle/peak resident memory, idle CPU, clock-update CPU, resize/route repaint time, editor input-to-display latency, and sustained operation. No values or device support are established yet. Compare the same station fixture and workload; use results to set budgets rather than invent performance claims.

Verify the app runs without root and opens no listening service. SSH can administer/start the executable but is not browser control. Local browser control remains a separate authenticated service batch and must not be silently enabled here.

Remaining risks: Qt deployment/licensing notices and source/replacement obligations, actual desktop System-theme behavior, touch text entry/keyboard occlusion, custom-map semantics, Pi memory/performance, generalized coastline detail, original callsign/input-policy differences, absent original projections, timezone/state model, and full provider-backed parity. Refer to `native-ui-decision.md`, `map-provenance.md`, and `station-clock-parity.md`; completing this dashboard does not close those gates.
