# Independent Qt world map audit

Status: **widget and preview integration independently verified locally; root desktop computer QA passed after corrections**. Reviewed 2026-10-03. Scope: frozen `world_map_widget.hpp/.cpp`, widget tests, `map_preview.cpp`, CMake integration and original geography helpers. No source or tests were changed by the reviewer. This bounded audit does not establish complete HamClock parity or release readiness.

## Finding resolved before approval

**P2, initial `world_map_widget.cpp` label clamp:** a nonempty map smaller than the font ascent could pass a lower bound greater than the upper bound to `std::clamp`, violating its precondition. The initial empty 12 × 12 bounds test did not exercise endpoint painting. The writer fixed this by omitting labels when bounds are shorter than the font height or narrower than the label. Six endpoint-render cases now cover 16 × 16, 17 × 17, 18 × 18, 33 × 17, 17 × 33 and 40 × 40. Re-review confirms the clamp preconditions hold after the guard.

## Independent evidence

The final frozen widget regression target was independently compiled against the installed Qt Core/Gui/Widgets frameworks with C++17, warnings, AddressSanitizer and UndefinedBehaviorSanitizer. It reported **157 map widget checks, zero failures**, exited zero and produced no sanitizer findings. Tests exercise aspect/bounds, analytic equatorial pixels, short-arc rendering, dateline segments reaching both edges without a world-spanning chord, undefined antipodes, invalid endpoint preservation, actual coastline pixels, theme ocean pixels and route visibility.

A separate independent sanitizer harness verified actual coastline painting with endpoints cleared: 15,688 pixels inside the map differed from the uniform ocean at the fixed 640 × 400 fixture. This guards against an empty geography layer that ocean-color tests alone would miss. It also verified invalid appearance rejection retains station descriptions, hiding the route retains the computed geometry while describing the hidden state, and actual QApplication palette changes repaint System mode from dark ocean to light ocean. The additional harness exited zero with no findings.

## Source review

- Geographic coordinates are validated before committing widget state. Route sampling is bounded by the shared 64-subdivision default; undefined antipodal routes draw no invented segment. Invalid mode changes throw before mutation.
- Coastline parts are iterated independently and adjacent vertices are seam-split before populating a cached normalized painter path. Separate `moveTo` operations prevent inter-part connections. Route segments use the same projector/splitting helper. Data provenance remains documented in `map-provenance.md` and independently checked in `map-audit.md`.
- Letterboxing preserves a 2:1 world aspect with eight-pixel margins. The map and marker drawing is clipped to those bounds. Circle versus square plus DE/DX text provides a non-color distinction. Markers at seam/pole edges may be clipped; nearby or coincident label overlap remains a visual limitation.
- Light/Dark ocean, coast and route choices are explicit. System determines its drawing scheme from the current Window palette and updates on palette events. The surrounding panel color comes from its containing widget palette; the map does not control an entire application's theme.
- The accessible name/description states projection, data origin, endpoint coordinates, distance/bearing, undefined geometry and route visibility, and emits a description-change event. This is descriptive metadata, not proof of screen-reader announcement or an interactive accessible map.

## Remaining gates

No remaining blocker was found for this bounded widget and preview source increment. Root must inspect native rendered coastlines/labels/themes before publication. Pi startup/CPU/memory/input, assistive software, additional projections, land fill, long path and live overlays remain unverified or unimplemented.

## Saved-station preview integration

Independent CMake build and both registered CTest regressions passed; direct Qt map execution reported the 157 checks above. Fresh `make verify` passed the native and executable checks plus all 31 importer methods.

The actual built preview rejects absent/unknown/extra/empty arguments, missing or malformed configuration, final symlinks and FIFOs with exit 2 before creating QApplication. An isolated executable harness exercised all those cases, confirmed no FIFO block with a timeout, and confirmed original settings bytes and symlink entry remained unchanged.

Source review confirms the preview accepts only an explicit valid saved station, displays its real callsign/grids and computed geometry, and performs no save operation. Appearance changes update the shared dialog helper and the map's drawing mode; route visibility changes only drawing/description. Closing the dialog writes nothing. Strings shown as labels explicitly use plain text. Controls have accessible names/descriptions, tab ordering and minimum 44-pixel widget heights; actual touch hit regions and assistive behavior remain unverified. The cached map is independent of controls and coordinates still pass shared geographic validation.

CMake's static `hamclock-world-map` target contains original application code; its Qt dependency targets remain dynamic Core/Gui/Widgets. No additional Qt module, copied font, network service or data provider was introduced by this integration.

## Accessible-value correction

Root native inspection found that generic accessible names on the standard grid and geometry labels suppressed their actual displayed values, and the custom map widget was absent from the macOS accessibility tree. The source was corrected so those label names include the actual grids and computed distance/bearing. The geometry label also receives the map's full description (projection, coordinates, shape distinction and visibility), refreshed whenever the route checkbox changes.

Independent narrow source review confirms the corrected text comes from validated station/geographic presentation and remains plain text; the checkbox callback updates the same existing description without saving or altering endpoints. A fresh CMake build and both CTest targets passed after this patch. Root is rechecking the final native accessibility values. The standard-label alternative is necessary here; source metadata on a custom widget alone does not establish platform exposure or screen-reader usability.

## Final desktop evidence and keyboard correction

Root's fresh native check confirmed accessible labels include actual DE `CM87SS`, DX `IO91WM`, distance 8,616 kilometres and initial true bearing 33 degrees. Light/Dark screenshots showed the real coastline and route correctly, and hiding the route retained coastlines/endpoints.

macOS default Tab behavior initially skipped the checkbox. The preview now requests `Qt::TabFocusAllControls` through `QApplication::styleHints()->setTabFocusBehavior`, scoped to the application. Independent installed-header inspection confirms the public setter exists in Qt 6.11.2, and a fresh build plus both CTest targets passed. Qt's [QStyleHints documentation](https://doc.qt.io/qt-6/qstylehints.html) describes programmatic adjustments as applying only to the current application. Exact Qt 6.4 compatibility was not independently verified: fetching that older header/documentation failed, so the declared minimum remains a target-build gate rather than a tested claim.

Root then verified actual Tab focus reaches the checkbox, Space hides and restores the route, Shift+Tab returns to the appearance combo, and Escape closes the preview. No remaining blocker was found for the bounded desktop preview. The custom painted map remains absent from the macOS accessibility tree; the standard summary label supplies the text alternative. Actual screen-reader use, physical resizing, Pi/touch behavior and real OS appearance toggling remain untested.
