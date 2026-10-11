# Independent countdown state audit

October11,2026. Reviewer owns only this report. Writer owns new countdown header/source/test; root owns shared build/Git and integration. No other source, dependencies, native UI, radio or shared files modified. Header gate pending writer freeze; cpp must wait explicit independent acceptance.

## Documentary scope

Previously independently rechecked primary [HamClock4.22 guide printed11/PDF13](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf#page=13), countdown lines532–536: adjustable countdown has a documented minimum of one minute; remaining time appears on main icon, an optional pane and Big Clock; clicking those active surfaces restarts; a three-second main-icon hold enters Stopwatch. These establish the relevant minimum/restart requirement but do not establish a numeric maximum, exact input increments, pause/resume/cancel semantics, expiry identity/acknowledgment or persistence. CT0 must label all such choices as original replacement policies. No UI, countdown native parity, alarms or hardware claim follows from pure model tests.

## Initial header gate finding

Initial header intentionally exposed only a pending Boolean without run identity. **API gate blocked:** root's bounded event-identity/counter-wrap requirement was not satisfied, and a delayed acknowledgment from an earlier run could clear a restarted run's event. Requested instance-scoped uint64 Start generation, atomic exhaustion rejection without wrap, pending tied to current generation and explicit acknowledgment-token matching; Reset preserves generation. Root confirmed full-range checked next-generation helper boundary tests instead of adding a public state seed or artificial small lifetime cap purely for testing. No cpp permitted before revised gate.

Other initial policies reviewed: documented inclusive minimum1minute, explicit7day replacementmaximum/fixed duration, Start fullrestart, Stop advancing then pausing unlessexpired, Resume pause exclusion, no negative remaining, ordered unsigned-span saturation at expiry, reversal/error atomicity and successful fixed allocation-free storage. Pause/cancel/ack/cap policies are not original documentary claims.

## Revised API gate

Revised header independently read; **explicit pre-cpp API gate passes**. Every Start uses a checked fulluint64 next-generation helper before commit; zero initially, Reset preserves generation, instance lifetime binding explicit. Zero/mismatching acknowledgment tokens reject without any clock/state commit; matching tokens advance then clear pending as an explicit replacement policy, including advancing exactly into expiry. Pending cannot re-arm after acknowledgment absent a new Start. Full-range helper exhaustion must be directly tested and actual shared apply integration independently reviewed; that is not evidence of executing2^64restarters. No seeding/backdoor/artificial small product lifetime cap introduced. Candidate-state duration/time/error contracts remain required source gates.

## Frozen implementation review and hostile checks

Actual source gate passes. Private invariants enforce configured/remaining nonnegative bounded durations and expiry only from Running. Ordered signed endpoint subtraction is performed as unsigned full-span arithmetic; if increment reaches remaining it saturates to zero/Expired/pending before any signed cast. Otherwise increment is below remaining and safe to cast/subtract. Paused/Idle/Expired exclude elapsed. Candidate state and every rejection precede assignment/last-observed commit. Start applies the checked uint64 increment to candidate before any commit; exhaustion cannot wrap/reuse identity. Reset retains identity. Zero/stale acknowledgments reject before elapsed/time changes, while matching acknowledgment explicitly advances then clears current pending. Expired phase prevents any re-arm without Start. Fixed successful state transitions allocate no heap; exception construction may allocate.

Independent strict C++17 warning-as-error ASan/UBSan **61 permanent checks pass**, zero failures. Separate **85 hostile probes pass**: independently derived128-bit oracle over36 signed timestamp pairs validates rejection/order versus saturated remaining/expiry, plus staleack after restarted expired event preserving pending/time, matching acknowledgment/no rearm, Reset identity preservation, fullrange helper boundaries/exhaustion. No sanitizer diagnostics. Complete-service UINT64 exhaustion was not dynamically reached; directly tested helper and independently reviewed precommit application establish the bounded implementation path, not an executed2^64-run claim. No copied original implementation or third-party code introduced; source provenance audit is scoped, not legal certification.

Root CMake wiring is standalone pure regression under BUILD_TESTING with15s timeout and warning flags; Make explicit headers/source and model-check compile/run reviewed. No native countdown activation, scheduler, dependencies, UI or network added.

Exact SHA256 header `94a721501ffd0e223f9ff0bd5200c5a3d0c905a1f05efa5539c642135e753b97`; cpp `6bfa50bfd5b10cc28e2f590aa3e5015dfeacbe537c7134df4caf22772a84196c`; permanenttest `d07c8a439eb6a60e4356f13d397cce6809ab0f447d33c959c3aa530a55d5e5a7`; CMake `daeed79708de3e70300103e60dca33403d94ed252531003fb5c97d63ea805c32`; Make `a3524b81f20a03ac11b9df27c71e847361e9ce7c71ff138234f449f3ff4fee88`. Whole fresh suite/validation doc gate pending.

## Whole-build and validation gate

Fresh independently configured/build tree `/private/tmp/hamclock-resumed-land-audit/countdown-cmake` passes **all19CTest**, zero failures,7.05seconds. Root separately reports19CTest3.70s and full makeverify. Actual source/build hashes match frozen writer/root values. Native UI unchanged and no computer-control acceptance claimed for this increment.

Root `docs/countdown-state-validation.md` reviewed against actual source/tests: identity/lifetime/remaining/minimum versus replacementcap/noUI/helperexhaustion distinctions accurate. Requested one precision correction: Stop pauses only if remaining positive, otherwise remains Expired at/beyond deadline. Broader new countdown-ui-research.md is separate pending primary/policy review and not promoted by this model gate. Fullservice counter-exhaustion execution, physicalPi/touch/minQt/suspend/concurrentaccess/persistence/sound/countdownvisuals and complete parity/legal/accessibility remain unverified. Exact next action: correct the Stop wording, freeze modelvalidation/publication gate, then independent presentation/control API review.

Final rootvalidation Stop wording independently rechecked: pause only when time remains; at/beyond expiry remains Expired. **Final bounded CT0 publication gate passes.** Source/tests/build unchanged; research/UI/policy/hardware gates stay separate.
