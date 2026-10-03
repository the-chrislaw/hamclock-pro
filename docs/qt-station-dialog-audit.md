# Independent reusable station dialog audit

Status: **bounded reusable-dialog increment passes independent source/regression review, with root native confirmation recorded; ready for draft review, not a product release**. Reviewed 2026-10-03. Scope: reusable dialog, thin editor entry point, modal regression tests and CMake integration. No production source or repository tests were changed by the reviewer. Work stopped at the agreed checkpoint without publishing a PR.

Root native verification confirmed successful Return Save and persisted fields, then found forward/reverse Tab navigation skipped Save between Appearance and Cancel. The writer is adding explicit button focus policy and an actual modal key-navigation regression. Hold final keyboard approval until that fix is frozen and independently rerun; explicit source tab-order calls alone did not prove the running native behavior.

The button focus-policy fix is now frozen and independently rebuilt: Save and Cancel both explicitly use `Qt::StrongFocus`. Fresh CMake build and all three CTest targets passed; a fresh standalone sanitizer run reported **30 modal checks, zero failures**. The added cases send real Tab/Backtab events and inspect the actual focus widget for Appearance → Save → Cancel and reverse Cancel → Save. The offscreen environment did not reproduce the original macOS failure, so root's native re-check remains necessary before closing that finding.

The writer then reproduced the platform-order reset deterministically using a Mac-button-layout proxy style and repaired explicit theme switches by reapplying the logical tab chain after styling. A further independent cross-feature fixture showed this was incomplete: while System is selected, changing QApplication's Window palette invokes the shared appearance observer, whose stylesheet refresh bypasses that manual callback. The Mac-layout harness again reported three focus failures in 36 checks. Restoring the chain must occur after every relevant button-box style rebuild, including live System changes; final approval remains withheld pending this correction.

The final frozen correction uses a local `LogicalButtonBox`: its StyleChange handler invokes the standard button-box implementation first, then restores the requested Appearance → Save → Cancel chain. The callback captures guarded `QPointer` references, is installed only after controls exist, and leaves other native button-box behavior intact. This covers explicit theme changes and observer-driven live System refreshes without changing the shared appearance API.

Independent re-review passed the final CMake build, all **four CTest targets**, and `make verify`. A fresh sanitizer compilation ran both ordinary and Mac-layout modal variants: **36 checks each, zero failures**, with no sanitizer findings. The permanent Mac-layout case includes the independently discovered live application-palette flip. The reproducible source/regression blocker is closed. A real OS theme change is not claimed by an application-palette fixture.

At the final checkpoint, root reported a fresh native rebuild with four passing CTest targets and confirmed Appearance → Tab → Save in the running editor. Root changed the callsign to N2CANCEL, clicked Cancel, observed the application exit, and independently compared the freshly captured baseline checksum: exact file bytes were preserved. Earlier manual theme forward/reverse navigation passed after the first ordering repair; the final live System style-change path is covered by the Mac-layout regression. These native observations are root-provided evidence, not actions performed by this reviewer. The earlier disposable-file baseline was stale and is deliberately excluded from the cancellation conclusion.

## Evidence

Fresh CMake build and all three CTest targets passed. `make verify` also passed native/executable checks and all 31 importer methods. A separately compiled C++17 AddressSanitizer/UndefinedBehaviorSanitizer harness ran the actual modal suite: **25 checks, zero failures**, with no sanitizer findings.

An additional independent modal harness extended those fixtures and passed **32 checks**. It verified closing a new draft by window close returns no settings and creates no file, correcting a failed latitude-91 draft then successfully saving returns the actual persisted result, and changing multiple fields/theme then canceling preserves exact prior file bytes. These are real dialogs driven through their event loop, not a copied Save implementation.

The actual rebuilt thin editor rejected missing/unknown/extra/empty arguments, missing or malformed files, final symlinks/FIFOs and an already-existing new destination with exit 2. The isolated CLI harness confirmed original file bytes and link remained unchanged. Invalid settings still fail before QApplication creation.

## Result and persistence review

`show_station_editor` owns its baseline snapshot and local draft. Only a successful Save sets its returned optional value. Validation or filesystem failures retain visible edits/error and leave the result empty; Cancel, Escape and window close discard the draft. Actual fixtures cover default Return Save, existing/new saves, 0600 mode, invalid retention, competing new destination refusal and missing-directory error.

The returned settings snapshot is allocated before publishing to disk. After successful publication, its optional is moved into the return state; `StationSettings` contains ordinary strings and value coordinates whose moves do not require another string allocation. This avoids the specific failure mode of constructing a return snapshot only after committing the file. Persistence remains delegated to the reviewed replacement/exclusive-create APIs, preserving their trusted-parent and crash-durability limits.

Callbacks are scoped to the local dialog and its modal event loop; no returned pointer references destroyed fields. Inputs, appearance helper, labels, error alert and scrollbar retain the previously reviewed validation and accessibility metadata. The thin executable retains explicit startup grammar and early load checks, and now requests application-local all-control Tab navigation using the same installed Qt API reviewed for the map preview.

## Limits and disposition

No remaining source or persistence blocker was found for the frozen reusable-dialog extraction after the focus fixes. The independent regression results and bounded root native confirmation support moving this increment to draft review on the next run. The regression's action callback can fail to close a nonexistent active modal; CMake's explicit 15-second timeout bounds that potential test hang. Offscreen warnings about size-hint propagation do not establish actual desktop sizing behavior.

This batch supplies a reusable editor function, not dashboard editing integration. Pi/touch behavior, actual OS theme changes, actual screen-reader error announcement, Qt 6.4 builds and distribution license obligations remain separate gates. Full HamClock parity, security certification and accessibility compliance are not established by this audit.
