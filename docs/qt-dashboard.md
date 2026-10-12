# Native Qt station dashboard

This original C++ Qt Widgets executable connects existing station settings, geometry, real coastline rendering, and the reusable station editor. It is a working offline station-tool slice, not complete final-original HamClock parity or the completed accepted visual design.

## Launch

```
hamclock-pro-dashboard --station /explicit/path/station.conf
```

The path must name a valid existing settings file. Syntax errors, empty/missing paths, corrupted/oversized settings, nonregular files, and invalid station values exit 2 before constructing QApplication. No fallback station, automatic overwrite, or default configuration is created. The shared loader rejects final-path symlinks; its trusted-parent-directory and file-size rules remain applicable. The same explicit path and validated snapshot are passed to the dashboard.

Qt receives only the executable name as an argument. App-local keyboard traversal includes controls. Standard system environment/plugin setup is still supplied by the deployment environment; command filtering is not a sandbox or plugin-security guarantee.

Create a configuration using the explicit station editor's `--new-station PATH` action or the existing validated CLI, then launch this dashboard. Closing the dashboard does not save anything implicitly.

## Working behavior

- Standard readable widgets display callsign, DE/DX coordinates and six-character grids, short-path kilometres, and initial true bearing.
- UTC date/time comes from the operating-system clock and refreshes each second. There is no independent synchronization-health certification, local-time conversion, or simulated-time control.
- The custom map draws Natural Earth 110m real coastlines with the existing equirectangular projection and seam splitting, distinct DE/DX markers, and calculated short-path geometry. Undefined bearings/routes are described explicitly. No solid land polygons or other original HamClock projections are claimed.
- “Edit station” opens the reusable native dialog. Only successful validation and persistence returns a saved snapshot and refreshes station labels, geometry, and theme. Cancel/Escape and save failures retain the current dashboard/configuration.
- Saved `system`, `light`, or `dark` appearance is applied. The shared System-theme observer follows application palette changes; actual desktop/session behavior still requires native verification. The editor preserves its logical keyboard order after manual and live theme changes.

No fake radio/provider values, map night shading, browser service, network listener, GPS input, radio transmission, or hardware control is added by this entry point. The displayed offline status and Natural Earth credit identify the current slice.

## Build and verification

The integrator owns CMake target registration. The dashboard target needs `src/qt/dashboard_main.cpp`, `station_dashboard.cpp`, `station_editor_dialog.cpp`, `editor_appearance.cpp`, and `world_map_widget.cpp`; pure station-edit/view, settings, geometry, map projection, and path sources; Qt Core/Gui/Widgets. Existing tests and the independent editor executable remain separately buildable.

Author verification: standalone dynamically linked compilation with C++17 warnings enabled; actual executable invalid-syntax/missing/corrupted-file checks under an intentionally invalid Qt platform environment prove rejection happens before QApplication startup. These checks establish startup behavior only. Root owns full build registration, dashboard integration regressions, native GUI acceptance, and publication.

Required remaining gates include the actual seven-inch Pi 4 touchscreen and Pi 3 build/performance, native keyboard/default actions and assistive-software checks, screen scaling/reflow, live System appearance, dialog keyboard occlusion, save/cancel interaction, and source/dependency distribution review. Standard-widget semantics and passing model tests do not establish ADA/WCAG compliance. Qt and its deployed components retain their own license obligations separately from original MIT application code. See `native-ui-decision.md`, `map-provenance.md`, `station-settings.md`, and `qt-dashboard-integration-plan.md`.

## Integrated evidence

CMake registers the dashboard executable and fifth CTest target. All five tests and root make verify pass locally. Independent review reran 115 corrected dashboard checks under ASan/UBSan (leak detection disabled). Native Save refreshed all fields/map/Dark appearance; invalid input and an actual temporary-file creation failure retained the draft, while Cancel preserved saved bytes. Current-value accessibility metadata was repaired after actual native inspection. Full observed evidence and remaining limits are recorded in `computer-validation.md`; none of these results establishes Pi or full feature parity.
