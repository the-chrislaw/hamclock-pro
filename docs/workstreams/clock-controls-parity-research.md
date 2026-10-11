# Clock controls: final-original evidence and next bounded slices

October 11, 2026. Research only, awaiting independent review. Only this document changed. The author's **HamClock4.22 User Guide**, dated January17,2026, is available through the [manual mirror](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf). No original source, screenshots, executable, live station or hardware was inspected/copied. Documentary behavior is evidence, not executable parity certification.

## Compact manual-derived evidence

Primary anchors: [printed4 / PDF6](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf#page=6), main-clock controls; [printed11 / PDF13](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf#page=13), Time/Stopwatch/Count down/Big Clock. Two front-matter sheets precede printed1.

**High confidence, documented:**

- Shifted UTC has flashing red OFF; reset returns to real UTC; unknown time shows a question mark. DE timezone correction is separate.
- Stopwatch uses hundredths. Run starts/resumes; Stop freezes counting/display; Lap freezes display while counting continues; Reset starts over; color control and Exit are available.
- Independent daily/once alarms select DE or UTC. Armed icon is green; main, timer and Big Clock surfaces announce firing; click cancels; alerts expire after60seconds.
- Countdown minimum is1minute; duration is adjustable. Remaining time can replace the main icon, occupy a pane or appear in Big Clock. Clicking restarts; three-second main-icon hold opens Stopwatch.
- Big Clock offers analog/digital, UTC/DE,12/24-hour, date, DE/space-weather, seconds and alarm/countdown options.

**Unknown from this inspection:** exact numeric maxima/step sizes, Lap unfreeze interaction, Reset while running, restart/suspend persistence, clock-offset increments, independent alarm overlap handling, timezone-change effects and DST gap/fold policy. Do not invent original semantics from the UI labels.

## Existing model gap assessment

Read `src/domain/stopwatch_state.hpp` without editing. Its original replacement contract has Idle/Running/Stopped, injected monotonic nanoseconds,64 bounded cumulative/interval laps, clock-order/overflow protection and atomic rejection. These are engineering choices, not original numeric limits.

A presentation latch is missing: the currently exposed elapsed snapshot keeps advancing after Lap. A recording list alone cannot satisfy the documented frozen-display behavior. Preserve the running domain clock and implement a separately tested view-state latch; resolution of the unfreeze gesture stays explicit rather than assumed.

The model intentionally excludes formatting, controls, scheduler, wall-time conversion, persistence, alarms and suspend detection. A monotonic source that pauses in system sleep cannot be corrected by this class. Domain tests therefore do not establish kiosk behavior, readable timing, hardware compatibility or alarm delivery.

## Proposed original implementation policies and acceptance

Everything below is a **replacement proposal**, requiring peer API review. These choices do not fill the documentary unknowns with claims about the original.

- Keep a single shared timer service with injected clock interfaces; every visible surface reads the same immutable state. Views cannot maintain competing elapsed counters. Rendering ticks update presentation, not accumulated duration arithmetic. Record the chosen host monotonic clock's suspend behavior as evidence.
- Preserve exact elapsed duration internally; display truncation to centiseconds is an explicit formatter policy, never round a future time into existence. Treat overflow visibly and keep prior state unchanged on invalid controls. Avoid a dynamic lap-history allocation in the first presentation slice.
- Route keyboard/touch commands through one command boundary. Use visible, named controls with focus and status text; do not require color perception or an undiscoverable gesture. Any compatibility hold gesture must cancel on release/move/closed view and use elapsed monotonic time. Opening/closing views cannot reset timers.
- Timer expiry creates an identified pending event once; acknowledgments clear the event across all consumers. Simultaneous event identities remain independent. No beep, GPIO, radio command, sleep prevention or system-clock modification is implicit in a UI event.
- Alarm scheduling uses actual UTC and declared civil-zone rules, never the preview/offset clock. Store configured intent separately from a transient firing event. Persisted rules require version/schema/atomic-write review; first pure model may remain nonpersistent and visibly disclose that limitation.
- Treat UTC absolute once-events and recurring civil rules as different contracts. For a user-entered ambiguous civil instant, surface two candidate instants and require an explicit occurrence choice. For a nonexistent instant, reject editing with a clear correction path. Proposed daily policy: skip a missing civil occurrence, choose the first repeated occurrence, and fire at most once per configured civil date. These are provisional product choices needing review, not manual behavior. Do not implement a daily civil rule as adding86400seconds to the previous UTC instant.
- After clock jumps/suspend/restart, explicitly decide missed-event handling; proposed conservative default is mark missed and require acknowledgment rather than emitting arbitrarily late notifications. Event identity includes rule revision and scheduled instant so rollback cannot refire the same occurrence. Rule changes invalidate outstanding candidates through generation guards.

[IANA's timezone reference](https://data.iana.org/time-zones/tz-link.html) describes changing civil offsets/DST rules and public-domain tz data, updated periodically. It cautions that rules may change with little notice and data is not authoritative legal time. Versioned rule evidence matters; a geographic timezone boundary layer is a separate provenance problem. Attempted Qt6.4 QTimeZone/QDateTime documentation retrieval failed through this browser. No claim that newer transition-resolution APIs work on the project's minimum Qt version is made; backend compatibility remains a research/build gate.

## Exclusive coding tickets

Root owns dashboard integration, shared build/settings and station/clock wiring. No file is assigned twice. APIs freeze before implementation; independent review and rerun are required for each slice.

| Ticket / exclusive new files | Bounded scope | Verification gate |
|---|---|---|
| CP0 `src/app/stopwatch_presentation.hpp/.cpp`, `tests/stopwatch_presentation_regression.cpp` | View-state latch and centisecond formatting over immutable model snapshots; no QWidget/timer/persistence. | Advancing underlying time while held display remains fixed; explicit release; reset/stop/repeated-lap interactions approved as product policy; capacity/overflow invalid-command atomicity. |
| CP1 `src/qt/stopwatch_dialog.hpp/.cpp`, `tests/qt_stopwatch_dialog_regression.cpp` | Keyboard/touch view consuming CP0 through injected service; no wall clock/network/hardware. | Deterministic injected ticks; focus/name/state; close/reopen preserves service; Light/Dark/System and7-inch sizing; root actual computer-control QA independently. |
| CT0 `src/domain/countdown_state.hpp/.cpp`, `tests/countdown_state_regression.cpp` | Pure bounded duration/event model, explicit start/restart/cancel, no scheduler. | Before/at/after expiry, exactly-once event generation, clock rollback/overflow, repeated restart, no negative remaining; decide product cap and policy before code. |
| CA0 `src/domain/alarm_intent.hpp`, `tests/alarm_intent_regression.cpp` | Distinct absolute/civil once and civil daily configuration types, UTC/manual-offset first; no rule backend or IO. | Strict civil validity, contradictory settings, explicit ambiguity policy, revision IDs, domain bounds; no claims of timezone-backed recurrence yet. |
| CA1 `src/domain/alarm_scheduler.hpp/.cpp`, `tests/alarm_scheduler_regression.cpp` | Injected candidate-instant provider and due-event evaluator, no OS clock. | Synthetic gaps/folds/nonhour offsets, overlapping identities, clock jumps/rollback, acknowledgment, deadline and rule changes. Backend/civil converter separately audited before real DE activation. |
| CB0 `src/app/big_clock_preferences.hpp/.cpp`, `tests/big_clock_preferences_regression.cpp` | Typed presentation options and bounded validation; no image/data acquisition. | Unsupported enums, dependencies unavailable, compact layout policy, actual-versus-preview indication; root later chooses exclusive QWidget files. |

## Two paths, two steps ahead

| Area | Path A | Path B | Recommendation / uncertainty |
|---|---|---|---|
| Timer UI | CP0 latch/formatter -> CP1 accessible dialog -> root shared-service dashboard integration. | Larger dedicated full-window timer view -> mirrored compact controls later. | A gives the smallest verifiable increment; B may match kiosk convenience but risks divergent state unless same service. No final composition approved in this research. |
| Recurrence | UTC/manual-offset candidates with explicit rules -> independent fake-transition scheduler -> reviewed platform zone adapter. | OS/IANA zone adapter first -> platform-specific gap/fold probing -> scheduler integration. | A keeps original pure code moving without falsely claiming DE DST coverage. B requires backend/minimum-version and rule-provenance gate first. |
| Restart behavior | Persist configuration only -> mark past due occurrences as missed -> separately approved active-duration restoration. | Persist running deadlines and captured clock basis -> audit crash/suspend reconstruction -> restore pending events. | A is smaller; B could support continuity but cannot assume a saved monotonic value survives reboot. Choose after explicit product review. |

Exact next action: independent agent verify manual page mapping and frozen-display gap, then propose CP0 API. CA0 can progress independently after enum/range contracts, while real recurrence waits for a verified civil-rule backend and reviewed policy. No full parity or accessibility certification follows from this document.
