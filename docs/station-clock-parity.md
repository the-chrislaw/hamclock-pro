# Station and clock parity: bounded behavior inventory

Research date: October 3, 2026. Agreed target: final original HamClock 4.22. This is a behavior specification draft, not full parity certification.

## Evidence and confidence

The archive [README](https://github.com/openhamclock/hamclock-client-archive) identifies 4.22 as the final CSI release and links its manual. The actual author-written [HamClock 4.22 User Guide](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf) was retrieved through a mirror; its cover identifies 4.22 and January 17, 2026. It has 19 PDF pages, including two front-matter pages. References below use **printed page numbers**, so printed page 1 is PDF page 3. Screenshot retrieval was attempted for PDF pages 3, 12, and 15; two failed. Text references below were inspected directly; diagram geometry and tiny graphical labels require later visual comparison.

Confidence: high for the explicitly stated manual behaviors below; medium for mapping those behaviors to replacement acceptance fixtures; unknown for uninspected implementation limits. Archive source files/API returned fetch errors, and an immutable commit/version-header check remains outstanding. The mirrored manual is not yet byte-compared with the archive PDF. It supports behavior research; it does not establish permission to redistribute text, screenshots, assets, or code.

## Verified manual behaviors and replacement gaps

Original-behavior column is deliberately condensed; use linked manual pages for full context.

| Area | Verified original behavior | Printed page | HamClock Pro now / gap |
|---|---|---|---|
| Callsign | 3–11 characters; digit required; only `/` punctuation. | 1 | Current settings/view accept 1–16 characters and hyphen. This is a **known policy mismatch**, not final parity. |
| Manual location | Signed degrees, hemisphere suffixes, or Maidenhead input. | 1 | Decimal-coordinate CLI/model exists. Hemisphere text and grid input are absent. |
| Location dialog | Coordinate/grid click opens editor; linked values update; invalid confirmation remains flagged. | 10 | Draft validation/cancel model exists. Native touch/keyboard widgets and linked input conversion remain absent. |
| Precision | Internal precision retained; display rounded; grid uses southwest corner. | 10 | Exact supported-double persistence exists; six-character output exists. Grid-to-location conversion and original presentation comparison remain absent. |
| Map selection | Popup sets DE/DX; middle-click or modified left-click directly sets DX. | 10 | Geographic rendering exists; location-selection interactions remain absent. |
| Header | Callsign, custom title, rotation, and configurable PTT text. | 10 | Loaded callsign label exists. Title/rotation/PTT integration remains absent. |
| Station panels | Local clocks, sunrise/set formats, panel choices, DX prefix, SP/LP toggle. | 5 | Labels and short-path geometry exist. Local clocks and these controls remain absent. |
| Setup preferences | Date order, unit systems, True/Magnetic bearings. | 2 | Current view uses kilometres and true bearings. Preference-dependent display is absent. |
| UTC | Main UTC permits time shifting; OFF marks simulation; reset resumes real time; unknown time marked. | 4, 11 | Main clock reads the operating-system clock. Simulation and synchronization-health states remain absent. |
| Timezones | Manual/automatic offsets; automatic DST; persistence; moving location resets automatic. | 13 | No timezone settings or conversion implementation exists. |
| Configuration entry | Padlock opens configuration management. | 4 | Explicit single-file CLI save/load exists. Native configuration-management UI remains absent. |

Each row is supported by the [4.22 manual](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf) at the named printed pages. Current replacement state was checked against `src/main.cpp`, `src/app/commands.cpp`, and the station settings/view/edit modules; no claim follows about the user's installed original version.

## Acceptance fixtures and next two steps

These are proposed replacement tests, not additional claims about the original implementation.

1. **Editor and identity:** test click/touch field focus, virtual and physical input, validation messages, cancellation, and restart. First add grid-to-coordinate conversion with independently calculated southwest-corner fixtures. Then connect that conversion to a live draft with an explicit accepted field as the authority, avoiding accidental circular updates.
2. **Coordinate precision:** preserve a fractional longitude near a grid boundary while displaying its rounded value; reopening must expose the exact stored coordinate. First test conversion and presentation separately. Then compare a controlled fixture with the 4.22 runtime and capture screenshots without station-identifying personal values.
3. **Station-driven map changes:** clicking a map should propose a location without modifying a persisted file until the application accepts the action. First define projection-specific inverse-mapping contracts. Then test date-line/pole boundaries and linked label/path recomputation using the same immutable station snapshot.
4. **Clock states:** use injected instants around midnight, leap day, year rollover, and an external clock correction. First build a clock model separating actual system time, synchronization quality, and an optional simulation offset. Then verify every consumer uses the intended clock so a preview cannot silently affect hardware control or data freshness.
5. **Timezone states:** test a half-hour/quarter-hour zone, DST gaps/folds, offline startup, retry bounds, and changing location with an existing manual override. First compare local timezone data against a remote lookup approach for accuracy, size, licensing, and offline behavior. Then define persisted timezone identity separately from a transient UTC offset.
6. **Saved profiles:** first pin the original archive and inspect `configs.cpp` plus its help/API documentation. Then enumerate save/restore/rename/reset scope and test profile failures with the current safe-file writer. Existing persistence tests cannot certify whole-application profile parity.

## Unknowns and decisions

- Original supported grid lengths, exact coordinate grammar, defaults, maximum precision, and every editor key mapping still need source/runtime confirmation.
- Whole-application saved-configuration restoration, naming constraints, overwrite prompts, reset scope, migration, and failure semantics are unverified here. The earlier release-history audit is a lead, not a complete behavior specification.
- Time-shift increments, date-order choices, bearing model, unit conversions, UTC sync thresholds, and authoritative network-provider behavior need pinned source comparison before matching them.
- **Product recommendation:** preserve an original-compatible callsign validator, with a separate bounded arbitrary display-title field. This keeps identity and presentation policies distinct. The user does not need to decide an implementation detail immediately; discuss only if a deliberately broader identity policy is desired.
- **Timezone recommendation:** retain manual override and automatic operation as separate states. Prefer reliable offline timezone rules if a licensed, Pi-appropriate geographic mapping can be established. A current implementation decision can proceed behind that interface while research evaluates both mapping/provider paths.

All new code should remain original. No archived implementation or manual asset was added to the application in this research batch. Native editing, accessibility, Pi performance, and full final-release parity remain independent acceptance gates.
