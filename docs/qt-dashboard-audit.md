# Native station dashboard independent audit

Date: October 3, 2026. Scope: original Qt dashboard integration, its actual reusable editor modal and saved-station consistency. This is a source and executable widget audit, not computer-control validation or Pi certification.

## Results

No blocking dashboard defect found in the reviewed snapshot. Confidence is high for the exercised normal Save, Cancel, invalid-input and filesystem-error flows. Publication remains dependent on the reusable editor owner freezing its native-platform keyboard/theme fixes and the coordinator testing the integrated application through computer control.

Built independently in `/private/tmp/hamclock-dashboard-audit` with AppleClang 21, C++17, `-Wall -Wextra -Wpedantic` and installed Qt 6.11.2 Core/Gui/Widgets. Build passed without compiler warnings. Re-ran the unchanged author regression: **68 checks, zero failures** on Qt offscreen. Temporary independent extensions passed **81 total checks, zero failures**. The same extended executable passed AddressSanitizer and UndefinedBehaviorSanitizer; leak detection was disabled for this Qt/macOS run, so this provides no leak-clean claim.

The independent additions verified exact negative DE/DX coordinate labels, successful System-mode save and matching disk/snapshot, live application-palette propagation to dashboard and map, window-close cancellation, and rejection of empty/embedded-NUL paths and invalid initial latitude. These additions are isolated audit fixtures, not committed regression coverage. Existing offscreen warnings about unsupported size-hint propagation are platform-plugin limitations, not proof of actual native layout.

## Reviewed behavior

- `station_dashboard.cpp:31–34` validates the supplied initial snapshot and explicit path before construction proceeds. The constructor accepts a snapshot rather than loading it; the CLI must load the same path before constructing it. This API does not itself guarantee that an arbitrary caller-provided initial snapshot matches disk.
- `station_dashboard.cpp:81–94` derives labels, map endpoints, route summary and appearance from one snapshot. It adopts the editor result only after successful persistence. Cancel and both tested failures preserve the dashboard snapshot and saved bytes. Values are actual geometry and UTC, not generated example telemetry.
- `station_editor_dialog.cpp` allocates the candidate return value before publishing settings, then moves it after successful persistence. Errors remain visible in the modal and expose their text with an accessibility Alert. Normal validation/filesystem failures do not close it or refresh the dashboard.
- `station_dashboard.cpp:77–79` uses a parent-owned one-second timer and updates only the clock label. The test observed advancing UTC and zero map paint events during its isolated timer interval. This does not measure real-device CPU use or prove absence of repaints during every platform layout event.
- Widgets/timer use Qt parent ownership; signal connections use receiver contexts. Dashboard captures are scoped to its lifetime, and the editor modal runs synchronously with local draft lifetime. No source-level dangling callback found.
- Data labels use plain text, coordinates retain signs, and computed true bearing preserves undefined antipodal/coincident semantics through existing domain/map code. No external requests, radio operation or additional dependency/assets were introduced by the dashboard.
- Sidebar scrolling, edit-button minimum height 44, labeled accessible values and map text description improve operability. Offscreen programmatic `click()` does **not** establish touch reachability, visual fit, VoiceOver/AT-SPI operation, keyboard traversal or ADA/WCAG compliance.

## Limits and follow-up

1. Integrated CLI build/argument tests, actual desktop Save/Cancel/theme switching and small-window interaction remain coordinator checks. The dashboard's nominal 800×480 client size alone does not prove fit on a 7-inch screen including window decorations.
2. The dashboard author test checks snapshot-derived fields and map description, but does not independently numerically verify every map endpoint or all coordinate labels. The independent negative-coordinate checks supplement this audit. Domain/map regressions remain necessary; UI tests are not substitutes.
3. System appearance and window-close checks added here are not yet repository regressions. Consider promoting them in a later bounded test batch.
4. Existing persistence assumptions still apply: trusted stable private parent directory, regular-file replacement, last-writer-wins, no parent-directory fsync or external-change reconciliation. The dashboard does not expand those guarantees.
5. Catastrophic allocation failure after publication can prevent full UI refresh; ordinary failure-path consistency is tested. No crash/power-loss recovery guarantee is claimed.
6. The reusable editor's native Mac StyleChange/Tab-order issue was being fixed concurrently. This audit built the version already containing `LogicalButtonBox` restoration; any subsequent dependency changes require a rerun before publication.

## Reviewed snapshot identifiers

SHA-256:

- `src/qt/station_dashboard.hpp`: `7ba2b91f0d57fee097dd7ca5f166ac6e2b175d056b5752ff0ec62bf435e636cf`
- `src/qt/station_dashboard.cpp`: `91ff3728a6f5e96ba0ff71acd970fe4eb35cfc84ab9142816274c033c7010ea7`
- `src/qt/station_editor_dialog.cpp`: `51d7c5f1650823c0c9066d193864f158d64b636a47ed0ae02c283c009db70b5d`
- `tests/qt_dashboard_regression.cpp`: `6e074a5b80e12d799f6ea95a63d597623994e4e272b74c2677b80da6159e1cb6`

No full HamClock parity, Pi 3/Pi 4 performance, security certification, accessibility compliance or binary distribution license clearance is established by this audit.

## Final integrated build audit

After reusable editor freeze and publication in PR #10 (`852a22b`), independently configured a fresh build at `/private/tmp/hamclock-integrated-dashboard-audit` and compiled every current CMake target, including `hamclock-pro-dashboard`. Compiler warning flags remained enabled; no compiler warnings were emitted. All **five CTests passed**, including ordinary and simulated Mac button-layout dialog regression and the actual modal dashboard regression. Independently reran `make verify`: native self-tests, executable CLI regression and all 31 offline map-import regressions passed. These are local macOS Qt **6.11.2** results; they do not establish the advertised minimum Qt 6.4 or Pi compatibility.

Reviewed the integrated CLI: strict `--station PATH`, saved settings loaded and validated before `QApplication`, only executable name passed to Qt, application focus policy includes all controls, and parent-owned dashboard lifetime extends through the event loop. The CLI now supplies the snapshot from the same path, resolving the constructor caller-consistency precondition in this application path. CMake links the dashboard only to the existing original domain/map/settings and Qt Core/Gui/Widgets components. No new downloaded assets or dependencies were introduced.

Executed **12 invalid startup cases** against the actual freshly built dashboard binary: no arguments, missing path, wrong flag, extra argument, absent file, empty path, malformed contents, empty file, oversized file, FIFO, final symlink and directory. All returned status 2 within the bounded timeout, left the valid fixture unchanged and rejected input before GUI initialization. The test deliberately set `QT_QPA_PLATFORM` to a nonexistent plugin: reaching GUI startup would have failed Qt platform initialization, so clean validation errors provide evidence of the intended startup ordering.

No new blocking integration finding. Computer-control results must still be recorded separately by the coordinator; CTest offscreen does not substitute for observed desktop navigation and physical Pi/touch validation. The earlier dialog freeze dependency is now satisfied for the following audited hashes:

- `src/qt/dashboard_main.cpp`: `0a303fcdaef42f19ff31f8c0bc94ba97e262825dfbf327f4bc20cb8bdbfdc04d`
- `CMakeLists.txt`: `bf1a9d61c6185318aa71c575e06737c774058b2f7f97e4fc2b287516fcf50115`
- `src/qt/station_editor_dialog.cpp`: `51d7c5f1650823c0c9066d193864f158d64b636a47ed0ae02c283c009db70b5d`
- `tests/qt_station_dialog_regression.cpp`: `309aea754b730133403c6e97470c7b79c23fac298d1245a9b2ad7ee9340de79e`

## Accessibility value correction and final rerun

The coordinator's actual native accessibility inspection identified a defect missed by the initial source/offscreen audit: explicit generic accessible names hid the current station values in the native accessibility tree. The author corrected labels to include a stable descriptive prefix and actual current text, updated the UTC accessible name on every tick, and copied the current map metadata into the route summary description. This finding demonstrates why source/widget tests did not replace computer-control inspection.

Independently reviewed the correction. Prefixes are initialized once using an internal property, so repeated Save/timer updates do not append duplicate values. All station values remain plain text and come from the same validated snapshot. Route metadata is refreshed after map endpoints. The updated regression queries actual `QAccessible` interfaces rather than merely checking assigned QWidget properties and verifies station values before/after successful Save and on unchanged failure/Cancel paths, plus clock-name advancement.

Rebuilt the integrated targets and independently reran all **five CTests successfully**. The corrected actual dashboard modal regression reports **115 checks, zero failures**. Independently rebuilt the same corrected source under AddressSanitizer/UndefinedBehaviorSanitizer and reran **115 checks, zero failures** with leak detection disabled. No new blocking source finding. These tests verify exposed current values; they do not establish screen-reader announcements, VoiceOver/AT-SPI usability or accessibility compliance.

Native visual/Save/invalid-input observations belong to the coordinator's computer-validation record. Do not infer Escape-to-exit behavior from this audit: window close was observed by the coordinator, while its Escape result was ambiguous.

Corrected snapshot hashes:

- `src/qt/station_dashboard.cpp`: `82fe42dae31b498eb8a4cf2b251111a528545d6342dcdf78eeea0e7b60bad1a8`
- `tests/qt_dashboard_regression.cpp`: `62cbe57e68914d361b51f561109cac2268e72bedb593d570093150220f59b731`
