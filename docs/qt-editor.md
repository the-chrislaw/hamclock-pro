# Native Qt station editor prototype

This original C++ dialog uses Qt Core/Gui/Widgets standard controls and the existing `StationEdit`/settings implementation. It contains no copied HamClock implementation, bundled fonts, Qt Virtual Keyboard, network service or radio control.

CMake provides a separate dynamically linked `hamclock-pro-editor` executable containing the Qt station editor, station editor/view model, settings and geographic domain sources. The original SDL executable remains independently buildable.

## Explicit commands and persistence

```sh
hamclock-pro-editor --station /explicit/path/station.conf
hamclock-pro-editor --new-station /explicit/path/new-station.conf
```

`--station` requires a valid existing configuration. Missing, malformed, oversized, nonregular or unreadable settings cause startup exit 2 with an error; the dialog does not silently invent defaults. `--new-station` requires a nonexistent destination and shows an explicitly unsaved draft containing N0CALL and zero coordinates. Those values are visibly described as placeholders, not loaded station data. No arguments or other command syntax prints usage and exits 2.

Save and the default Save keyboard action copy bounded fields into `StationEdit`, validate through the shared model and invoke the shared atomic-temporary-file settings API: `save_new_settings` for a new destination and `save_settings` for an existing configuration. A successful save closes the dialog. Validation/filesystem failures remain visible as text and leave draft edits available. Cancel/Escape/window close write nothing. The executable returns 0 after dialog closure, whether saved or canceled; it does not imply a save occurred.

The startup existence check provides an early error. New saves atomically publish the completed temporary file using an exclusive hardlink operation: a destination created while the editor is open causes failure without replacement. Existing-edit mode intentionally replaces the selected regular settings file. Use a private, stable directory you control; directory crash durability and adversarial parent-directory mutation remain outside this bounded implementation. Unsupported hardlinks fail safely.

## UI and accessibility scope

The form uses system fonts, restrained opaque surfaces, native labels with associated fields, explicit accessible names/descriptions, 44 logical-pixel minimum control heights, defined tab order and a scrollable form for the seven-inch target. Field maximum lengths match the model's 16-character callsign/64-character coordinate limits; UTF-8 byte-length and valid ASCII semantics remain validated by the shared model. Errors are visible words, not color-only signals.

Follow system/Light/Dark changes preview a dialog palette and are stored only on Save. System resolves the current application palette and a lifetime-scoped observer refreshes styles on ApplicationPaletteChange, without Qt 6.5/6.8-only color-scheme APIs. Explicit Light/Dark palettes and original style rules apply no arbitrary external stylesheet URLs. System appearance reporting still depends on the actual OS/desktop integration and needs hardware/session verification.

The preview has no map/dashboard or on-screen keyboard integration. Touch text entry depends on the desktop input method or external keyboard. Standard-widget accessibility metadata is a foundation, not a completed screen-reader audit or an ADA/WCAG compliance claim; dynamic error announcement and focus behavior must be checked with actual assistive software.

## Verification and release gates

The user approved installing Qt base and CMake. This prototype builds locally with Qt 6.11.2 and CMake 4.4.3; independent inspection confirmed dynamic Qt Core/Gui/Widgets linkage. Native unit, executable and importer regressions pass. Computer testing caught and fixed explicit-Light control and nested-form defects on a dark macOS desktop. The permanent Qt fixture passes 93 checks, including rendered surfaces and live application-palette changes in System. Independent build/tests/review and fresh native Light/Dark/System, Save/Cancel, validation and filesystem-failure checks pass. Actual OS appearance changes, screen-reader announcements and Pi/touch tests remain pending.

Required tests: valid existing settings loaded, malformed/missing file fails before GUI, explicit new draft, saved fields round-trip, invalid coordinates/callsign keep draft and old file, filesystem failure keeps dialog open, Cancel/Escape preserve existing bytes, new destination appearing during editing is refused, theme changes cancel without altering stored choice, keyboard tab/default actions and labeled fields with a screen reader, 800 × 480 enlarged text/touch and desktop keyboard occlusion.

Qt licensing is separate from original MIT application code. Limit modules to the reviewed LGPL Core/Gui/Widgets set, dynamically link, inventory actual package/version/transitive notices, supply applicable license/source/replacement materials and verify target deployment. See `native-ui-decision.md`; no legal clearance or Pi performance claim is implied.
