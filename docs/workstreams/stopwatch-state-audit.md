# Independent stopwatch subset audit

October11,2026. Reviewer owns only this report; no writer source/build/Git/dependency/UI changes. Concrete stopwatch header/source and independent build gates completed below. Scope is original pure offline stopwatch subset, not full timer/alarm/clock parity or live/UI behavior.

## Parallel Kp semantic research verification

Independently read frozen docs/workstreams/kp-semantic-adapter-research.md SHA256 `268dce3824d98dcebb42868b9da5e2b2a982078000d921f4ef497a6b981400d4`. Directly rechecked [SWPC Station K/A](https://www.spaceweather.gov/products/station-k-and-indices) and [3-Day Forecast](https://www.spaceweather.gov/products/3-day-geomagnetic-forecast) primary descriptions. These support preliminary station monitoring, prescribed three-hour intervals, separate station a/daily A/planetary Ap and estimated-versus-official Kp; forecast and observed products remain distinct. Previously independently reviewed SCN26-21 and NWS policy support syntax migration and qualified reuse scope.

No retrieved primary field dictionary establishes this endpoint's zone-less time_tag bin alignment, a_running computation or station_count confidence semantics. This is limited evidence availability, not proof metadata does not exist. Author's medium-confidence UTC inference and named unresolved live gates are appropriately qualified. Minute chart cadence does not establish instant observations, exact wire refresh or unchanged-data freshness. Partner credit does not itself imply restriction on derived NOAA estimates or clear partners' raw datasets.

**Research gate passes for KS0 replay-only API proposal**, followed by independent concrete API and code gates. Not approval for live timestamp/interval/quality/right semantics. Synthetic replay must remain explicitly labelled, caller-declared time basis distinct from provider-confirmed UTC, raw null metadata preserved and source identities not inferred from arbitrary labels. Forecast remains separately researched. No scientific/legal/hardware/full-parity certification follows.

## Concrete stopwatch API gate

Actual header independently read before implementation. **API gate approved:** injected monotonic nanoseconds with arbitrary signed initial values, Idle/Running/Stopped transitions and explicit no-op command versus time-advance distinction; Resume excludes stopped intervals, Start requires Idle/Running; Lap records cumulative/interval elapsed without stopping, fixed64capacity rejects without overwriting. Reset explicitly validates monotonic order but discards elapsed without overflow arithmetic, permitting recovery from an unrepresentable running interval. Every rejected command/advance preserves the whole snapshot and last-observed instant. Borrowed snapshot lifetime and fixed allocation-free storage are explicit.

Implementation must validate enums/phases and safe subtraction/addition across the full signed timestamp range before commit; independent hostile boundary/unit/lap-cap/pause-recovery tests remain pending. The header correctly omits wall-clock/scheduler/countdown/alarm/display/persistence and suspend correction. Actual source gate pending.

## Implementation and build gate

**Bounded state/source gate passes, no blocking finding.** Actual frozen cpp independently reviewed: order validated before unsigned signed-endpoint subtraction, increment checked against remaining int64 elapsed capacity before cast/addition, candidate snapshot committed only after all validations. Lap intervals subtract monotonic cumulative nonnegative totals; fixed-cap append is checked before advancing. Wrong-phase/enum/reversal/cap/overflow failures preserve last-observed and every lap cell. Reset validates order and directly zeroes discarded state without computing a potentially overflowing duration. Fixed successful state transitions allocate no heap; standard exception construction may allocate, now accurately qualified in header.

Fresh strict C++17 warning-as-error ASan/UBSan **174 permanent checks pass**. Separate temporary **132 independent hostile probes pass**, including independently derived128-bit subtraction oracle across64 signed endpoint combinations, rejection-state atomicity, long stopped interval excluded, invalid enum,64-lap failure preserving all cells/time, and full-span overflow Reset recovery. Zero failures/sanitizer diagnostics.

Reviewed root build wiring: standalone dependency-free stopwatch CTest target with15second timeout; Make model-check compiles/runs state regression alongside other models. Fresh independent scratch configure/build `/private/tmp/hamclock-resumed-land-audit/stopwatch-cmake` builds all targets; **all16CTest tests pass**, zero failures,5.91seconds. No dependencies added, native app not launched and no UI acceptance claim for this model-only increment. Root reports separate full make verification; this reviewer independently checked fresh complete CMake suite and sanitizer executables.

Header `a6f010e5814dc9ba5331444c8d367f2ded95e16e78747ee531247d6458d43fed`; cpp `31b35814c7f962754e9c7c70a52cabf05604e4d3a73e08fddb15c80903b86955`; permanent test `ec4fac07ce05e95188ea92efe29ab6734458b067328e0341fbf951120e762676`; CMake `41d8295cf05cfbeb453ca88507f8c2cbb8b2fc6fe5b8b8047ae62e6752ff63b0`; Make `18dbac334a749479d476c4c28aaa55108548d9d10cba705e45716460bece5855`. Rechecked after execution.

## Explicit original behavior gap

Independently read [HamClock4.22 guide, printed page11](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf#page=13): original Lap freezes the displayed elapsed while counting continues, and its Run control starts or resumes. This state subset records laps/elapsed but supplies no presentation latch or original control mapping. An original independent presentation adapter must provide those behaviors, formatting/color/Exit and later native touch/keyboard tests. Countdown, daily/once alarms/timezones/acknowledgement, Big Clock, persistence and suspend behavior remain separate unfinished work. No original timer parity, UI accessibility, physical Pi performance or legal certification follows from this bounded offline model. No original implementation copied; primary guide used as behavioral evidence only.

Exact next action: root may publish this bounded state model after its checkpoint/verification gate, then queue independently reviewed display-latch/control adapter; live Kp semantic gates remain separate as documented above.

## Root validation wording recheck

Independently reread `docs/stopwatch-state-validation.md` after the root correction. Successful transitions are specifically qualified as having no heap allocation; rejected-operation standard exception construction may allocate. Test evidence, separate model/UI scope, original Lap display gap and remaining hardware/accessibility/legal/parity gates agree with this audit. **Final bounded model publication gate passes.** Broader clock-controls research remains a separate next gate and is not promoted by this report.
