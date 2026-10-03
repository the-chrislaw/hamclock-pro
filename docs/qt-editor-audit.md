# Independent Qt station editor audit

Status: **bounded prototype increment verified locally after fixes; root native computer re-check passed**. Reviewed 2026-10-03. Scope: `src/qt/station_editor.cpp`, appearance helper/regressions, CMake integration and the newly wired `save_new_settings` API. No production source was changed by the independent reviewer. This is a bounded prototype review, not release or accessibility certification.

Root runtime inspection subsequently found black fields in explicit Light mode. This is a real rendering defect; the earlier color calculations below validate intended values only and did not prove the displayed widget colors. A shared appearance helper and regression tests are being implemented. Do not describe the editor's appearance as verified until the repaired helper and actual widget rendering pass re-review.

## Appearance helper findings and resolved local regressions

The first frozen shared helper built successfully; CTest and its 73 offscreen palette/render checks passed independently, as did `make verify`. Literal Light/Dark control styles address the original black-input problem. Root actual native inspection then found that the nested scroll-form background remains black in Light mode; a nested-widget regression is being added.

An additional independent harness extended the frozen test with a live QApplication palette change after Dark/Light/System cycles. Its expected light application Base/Text colors did not reach the System input palette or rendered center. It reported three failures in 76 checks. Thus static return-to-System checks alone do not establish live system-follow behavior. Refreshing style/palette on application palette changes or equivalent handling is required, and both actual nested rendering and live-System regression must pass before appearance approval.

Root separately reported that invalid latitude 91 keeps edits and an accessible error visible, and a real Save closes the dialog with matching settings/0600 permissions confirmed through the native CLI. These are reported GUI evidence, not an independent screen-reader announcement test.

After the second helper freeze, independent CMake build and CTest passed. Direct offscreen execution reported **93 appearance checks, zero failures**, including rendered nested-form backgrounds, live application-palette changes in System mode, stable explicit overrides, invalid-mode preservation and one lifetime-scoped observer. The exact independent 76-check harness that previously found three live-System failures was rebuilt against the repaired helper and now reported zero failures. Fresh `make verify` also passed.

The helper now resolves bounded literal styles from the current palette, including descendant widget surfaces. A dialog-owned observer listens only for QApplication palette changes while System is active. It uses a weak dialog pointer and refresh guard; repeated application reuses one observer and replaces styles instead of appending. Explicit Light/Dark modes ignore application-palette refresh. Source review found no additional blocker. Root must still confirm repaired nested backgrounds and transitions in the actual native style; offscreen rendering is narrower evidence.

The current error path sets the visible plain-text message as accessible name/description and emits `QAccessible::Alert`. The dialog starts at 620 × 430 with scrollable fields and 44-pixel minimum input/button heights. These source changes are sensible but do not independently prove screen-reader announcements or 800 × 480 layout behavior.

Root reported a fresh final native GUI check: Light now displays white inputs and warm neutral nested form surfaces; Dark is consistently readable; return to Follow System exposes the expected accessible selection. Cancel closed without persisting previewed appearance. Earlier same-handler Save and invalid-coordinate checks remain applicable. This closes the native-style rendering defects for the bounded desktop prototype. A real OS appearance toggle, screen-reader error announcement and Pi touch layout were not tested; the live application-palette change is covered by the automated regression only.

## Confirmed local evidence

- The actual built `build/qt/hamclock-pro-editor.app/Contents/MacOS/hamclock-pro-editor` rejects missing arguments, unknown flags, missing/corrupt existing settings, existing new destinations, empty paths and extra arguments with exit 2. These failures occur before QApplication creation.
- A fresh AddressSanitizer/UndefinedBehaviorSanitizer persistence harness passed the settings tests and independent exclusive-create checks. An existing regular destination retained its original settings; existing symlink and FIFO entries remained intact. A successful new file had mode 0600 and one remaining hardlink after temporary cleanup.
- Two forked creators raced for one absent destination. Exactly one succeeded and one returned the expected filesystem failure; the final file contained one complete valid configuration. No leftover temporary files remained. The harness exited zero with no sanitizer findings.
- Fresh `make verify` passed native, executable and 31 importer regressions.
- `otool -L` shows dynamic QtCore, QtGui and QtWidgets frameworks, version 6.11.2, alongside system libraries. CMake requests only those three Qt modules; no Qt Virtual Keyboard or copied font asset is introduced.

## Save, Cancel and input review

The formerly known new-file race is addressed: creating mode calls `save_new_settings`, which publishes the fully written, synchronized same-directory temporary file with `link(temp, final)`. Existing final entries cause atomic failure rather than replacement. The startup existence check is a user-facing early error, not the security boundary. Existing-edit mode intentionally invokes replacement `save_settings` only after validation.

The same trusted, stable parent-directory assumption applies as in the settings audit. Hardlink support is required for new publication; unsupported filesystem operations fail rather than silently falling back to replacement. Parent-directory crash durability is not guaranteed. Temporary unlink failure after successful publication can leave an owner-only duplicate; it deliberately does not falsely report that committed settings were not saved.

Line edits cap displayed text at 16 characters for callsign and 64 for coordinates. Save converts to UTF-8 and the shared model separately enforces byte limits and permitted ASCII/numeric semantics, so multibyte pasted text cannot bypass model limits. The enum-backed theme combo supplies only System/Light/Dark data. All user-facing error labels use plain text; no user-selected rich text or stylesheet URL is evaluated.

Save/Enter validates all fields before file I/O. Validation or file errors keep the dialog and current text available. Cancel, Escape and window close perform no save; the local draft is discarded. Exit zero indicates dialog closure, not proof of saving. Qt-specific plugin command-line options are not passed through the explicit application grammar; normal local Qt deployment environment/plugin loading remains a trusted deployment concern.

## Theme and accessibility scope

The source uses standard line edits, combo box and dialog buttons with explicit accessible names/descriptions, label buddies, ordered tabs, 44 logical-pixel minimum heights and a scrolling form. Save is the default button. System uses an unresolved palette inherited from QApplication; Light and Dark preview explicit dialog palettes. Dynamic OS appearance changes and the effect of native style rendering still require runtime inspection.

Independent calculations of intended explicit palette colors give normal text contrast about 11.6–13.8:1 and selected-text contrast 6.9:1 in Light / 8.3:1 in Dark. Those values validate specified colors only, not actual widget pixels, all disabled states or full accessibility compliance. Error announcement, invalid-field focus, screen-reader navigation, focus visibility, 800 × 480 reflow and touch text input need actual assistive/hardware checks. No on-screen keyboard or dashboard integration is included.

## Dependency and release gates

Qt's [licensing documentation](https://doc.qt.io/qt-6/licensing.html) distinguishes module licenses and third-party components; its [Widgets module page](https://doc.qt.io/qt-6/qtwidgets-index.html) offers LGPLv3 as an option. The inspected installed package includes an exact-version SPDX inventory at `/opt/homebrew/Cellar/qtbase/6.11.2/share/qt/sbom/qtbase-6.11.2.spdx`. Published rolling web documentation is not a substitute for the exact distribution's license materials.

Before binary distribution, inventory actual target Qt libraries/plugins/transitive components, include required licenses/notices and appropriate source/replacement materials, and verify the deployment terms. Dynamic linking and module restriction are useful evidence but do not alone establish compliance. No full legal clearance, Pi performance, original HamClock parity or ADA/WCAG claim is made.

No remaining blocking defect was found for this bounded prototype after exclusive-create and appearance fixes. Independent code/regression evidence and root desktop GUI checks support this draft increment; target hardware, real assistive-software validation and distribution obligations remain release gates.
