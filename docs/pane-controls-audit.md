# Independent pane keyboard controls audit

Status: **locally verified for this bounded keyboard-control increment; root computer QA pending**. Reviewed 2026-10-03. Scope: frozen `src/app/pane_controls.*`, tests and main's controller, focus rendering and SDL key-event integration. The independent reviewer changed no production code or tests.

## Evidence

Fresh `make verify` passed native self-tests, executable regressions and all 31 importer test methods. A separate C++17 controller harness compiled with warnings, AddressSanitizer and UndefinedBehaviorSanitizer passed with zero failures and no sanitizer findings. Additional independent fixtures exercised activation without focus, entry and wrapping in both directions across all three panes, invalid directions and indices leaving state unchanged, selection wrapping from four to zero, normalization of five and maximum unsigned selection values, and bounded results.

A second standalone harness included the actual main translation unit with its entry point renamed, so it directly tested the real `Model::key` glue instead of a copied dispatcher. Return, keypad Enter and Space without focus did nothing; left and right Shift modifiers both selected reverse Tab navigation; unmodified Tab and arrow keys moved focus; all three activation keys cycled the selected pane. Unknown letter, up/down and function keys returned false without altering focus or selection. This harness also exited zero.

Static event-loop review confirms SDL key-down events pass the actual key symbol and modifier bits to the dispatcher and mark the display dirty when state changes. Escape still exits. Mouse pane selection uses the same controller and focuses/cycles only the hit pane. No filesystem, network, settings or route mutation was added by keyboard navigation.

## Rendering bounds

The two-pixel focus outline lies within the 800 × 480 logical canvas. Pane gaps remain visible: the expanded outlines of neighboring panes do not overlap. The map begins below the pane row, leaving space below its expanded focus outline. The maximum pane title and preview subtitle fit the 190-pixel pane at the current six-pixel character advance. Sidebar keyboard instructions fit its 183-pixel width and remain vertically separated from station fields.

Coastline/path loops, station presentation and marker projection remain unchanged by this increment. They continue to use the separate station/map validation already recorded. Public controller fields permit direct caller mutation, so callers must preserve valid selection indices before indexing title arrays; current runtime input paths preserve that invariant.

## Limits and disposition

No blocker was found for the bounded draft keyboard increment. Root computer QA should confirm the visible outline and actual input behavior in the running application before publication. Local controller tests do not establish Pi touchscreen behavior or prove display scaling.

This adds keyboard access to the existing three pane controls only. SDL still lacks a native semantic accessibility tree in this foundation; screen-reader access, readable production typography, full control coverage, native appearance modes and accessibility compliance remain unverified. This audit grants no full product release or accessibility certification.
