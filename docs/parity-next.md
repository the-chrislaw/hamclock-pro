# HamClock Pro — next implementation backlog

Prepared October 2, 2026 from `outputs/hamclock-pro-feature-audit.md` and `outputs/hamclock-pro-project-brief.md`. This is a bounded development plan, not evidence of implemented features or complete parity.

## Scope and evidence status

The agreed acceptance baseline is the final original HamClock 4.22. Retain its layout, controls, and functions while modernizing presentation. Pi 3 support is required; Pi 4 and Zero 2 W remain intended targets. Actual hardware measurements are required before declaring any board supported. The older project brief's Pi 4-first wording must not exclude Pi 3.

The audit identifies twelve release-history requirements as confirmed, including two simultaneous satellites, saved configurations, Wi-Fi history, TLE controls, GPIO REST controls, and DXPeditions. Its thirty F01–F30 families are module-backed candidates, not fully established behavior specifications. In particular, exact station, clock, locator, setup, and DE/DX calculation behavior still needs a manual/source crosswalk. A first useful implementation can proceed with explicit provisional decisions; those decisions must not be labeled original behavior.

Baseline capture remains a release blocker: pin an immutable archive revision, inspect the version identifier, retrieve the actual `doc/HamClockKey.pdf`, and inventory pane/map/setup/API choices. The project brief's `HamClockUserGuide.pdf` URL is known to be incorrect. Reference: [initial audit](../../outputs/hamclock-pro-feature-audit.md), [original archive](https://github.com/openhamclock/hamclock-client-archive).

## First functional vertical slice

Deliver one native screen with live UTC, editable DE and DX locations, calculated distance and initial bearing, and persisted station setup. Maintain the familiar station header, left DE/DX region, pane region, and map region. Unimplemented regions must state that they are unavailable instead of presenting simulated readings as live data. The prior HTML mockup is design evidence, not the native runtime.

| Priority / work item | Concrete result | Acceptance evidence | Original parity status |
|---|---|---|---|
| P0 — UTC clock | Read system UTC and refresh the display without accumulating timer drift; show date across midnight. | Controlled clock fixtures around midnight, leap day, year rollover, and a system-clock adjustment; a running native screen. | F02 candidate; exact original date formats and actions pending audit. |
| P0 — station setup | Edit callsign and signed DE/DX latitude/longitude with explicit N/S/E/W meaning; show validation errors and cancel without changing state. | Reject nonfinite values, out-of-range coordinates, malformed text, and excessive input; verify accept/cancel and restart. | F01/F25 candidates; original defaults and field limits pending. |
| P0 — persistence | Save validated settings through a versioned local format with atomic replacement; survive malformed or missing files with a visible recoverable default state. | Round-trip, interrupted-write strategy review, corrupt-file fixture, explicit storage path and file permissions. | Basic persistence is a slice requirement; full restore/rename/reset is confirmed and remains separately tracked. |
| P0 — DE/DX geometry | Compute great-circle distance and initial bearing from original, documented mathematics; display units and explain coincident or antipodal ambiguity. | Independent published/reference fixtures plus identical-point, equatorial, date-line, polar and near-antipodal cases; document radius, tolerances and bearing convention. | Calculation/UI details are provisional until audited; no assumption of exact original Earth model. |
| P0 — UI integration | Changing either location immediately updates its labels and geometry; clock remains responsive while editing. | One end-to-end exercise: launch, edit DE/DX, confirm result, cancel a second edit, restart, confirm settings retained. | Composition accepted in principle; exact geometry, hit regions and input mappings pending 4.22 comparison. |
| P1 — locator | Add original Maidenhead conversion with tested hemisphere, square-boundary and longitude-wrap behavior. | Round-trip/known-coordinate fixtures with stated cell precision and boundary convention. | F03 candidate; supported locator lengths pending audit. |
| P1 — local time | Add explicit timezone selection and clear unavailable states, with DST-aware conversion from UTC. | DST transition and offline startup fixtures. | F02 candidate; 4.22 timezone network retry fix confirmed. Do not substitute fixed offsets as full parity. |

Treat distance, bearing, and UTC as real calculated/system values. Do not claim sun/moon positions, propagation, spots, satellite predictions, weather, or working map projections until independently implemented and validated. A schematic map may support location placement only when clearly identified; real geographic assets require recorded provenance and permissions.

## Ownership and integration

The integrator owns build files, startup, public contracts, dependency choices, and final integration. Calculation work owns only its assigned domain directory; UI work owns its assigned UI directory; persistence/platform work owns its assigned directory. Research owns documentation. Freeze field types, units, command semantics, and failure states before parallel consumers implement them. Each implementation batch gets exclusive file ownership; shared-file changes return to the integrator.

Suggested handoff contract: validated station locations and callsign, a UTC snapshot supplied by an injectable clock, a geometry result with explicit validity/status, and commands to propose/accept/cancel settings. This is a proposal, not a frozen ABI. All calculations stay independent of rendering and network providers.

## Pi 3 verification gate

- Build the native application for the chosen supported Pi OS/architecture and record that environment; do not infer Pi 3 compatibility from a desktop build.
- Run the same slice on a real Pi 3 at the original reference layout size and the user's intended display size. Record startup time, resident memory, idle CPU, clock-update behavior, and input responsiveness. Select measurable budgets after a baseline run; no benchmark results currently exist.
- Verify that setup editing and clock updates remain responsive during save failure and corrupt configuration recovery. Avoid busy polling and whole-screen repaint work when only clock digits change.
- Verify the package runs without root and has no unintended listening network services. This offline slice needs no credentials or external data provider.

## Research stream that continues alongside implementation

1. Pin 4.22 and reconcile every setup field, pane choice, map mode, keyboard/mouse/touch action, and API command with manual pages and source symbols. Keep observed original behavior separate from replacement design choices.
2. Expand every F01–F30 family into behavior-level records using the audit template. High-priority later batches are astronomical calculations/maps, satellite tracking, propagation/space-weather providers, then spots/logging and hardware interfaces; priority does not remove any feature from scope.
3. Carry all confirmed revision-history details into those records: persistent satellite selections and two-satellite operation; Moon menu exclusion; rise/set azimuth and RSS label clearance; TLE age/user files; Live Spots path visibility; Ctrl-click rotation; persistent full-screen EME; Dst pane; GPIO REST; DXPeditions/RTTY/logger UDP; configuration restore/rename/reset; Wi-Fi history and bounded timezone retries.
4. For each external feed, record current primary-provider documentation, permissions, attribution, units, refresh policy, stale-state behavior, and parser bounds before wiring it to a live pane.

## Legal/security gates for this slice

Write original implementation code. Record the provenance and license of every dependency, font, asset, and later data feed; an MIT application license alone does not establish reuse permission. Do not copy archived original source into the repository. The baseline research can describe behavior, but any reuse proposal needs a separate documented permission review.

Bound editable inputs and stored configuration size; never evaluate configuration as executable code. Keep logs free of sensitive values by default. Handle file errors visibly and preserve recoverable settings. Avoid privileged execution or network exposure for the initial slice. Review these properties alongside functional changes, not only before release.

Completion of this slice means a useful native clock/location tool with tested setup and calculations. Full HamClock parity, legal clearance, security assurance, and supported Pi performance remain separate evidence-based acceptance gates.
