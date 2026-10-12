# Independent offline clock and spot model audit

Status: **C1 and D1 frozen source/regressions pass the bounded offline-model gate; root build integration pending**. Reviewed 2026-10-03. Exclusive reviewer file ownership: this report only. No production source, repository tests, build files or Git state changed. Scope: C1 clock state and D1 spot event/store; no network, provider, parser, persistence, UI or hardware integration is approved.

## Contract checks before implementation

C1 separates signed POSIX UTC seconds, monotonic nanoseconds, offsets and elapsed durations into distinct wrapper types. UTC can jump independently; monotonic time may repeat but not reverse. Preview, including a zero offset, remains explicitly marked and cannot change actual time, elapsed duration or sync quality. Advance/preview additions must reject overflow with prior state preserved. Fresh caller-supplied Unsynchronized evidence must remain distinct from Unknown; missing, future or stale evidence cannot yield synchronized status.

Reviewer requested explicit constructor rejection of negative maximum-age policy and clarification that Unknown describes sync evidence, not an absent wall-clock instant. The mandatory actual UTC argument cannot represent absent time; zero must not be used as an unavailable sentinel. Original unknown-time display remains a separate source/integration gate. Direct invalid-enum, signed-extreme arithmetic and full snapshot rollback tests are required.

D1 defines bounded plain ASCII identity/callsign/provenance metadata, uint64 frequency, optional valid geographic location and distinct reported UTC quality. Source/event identity keying is provisional storage identity, not proof of original same-call/frequency/latest-spot dedup parity. Unknown location must not fabricate a marker; Unknown time quality must not fabricate an age. Full signed int64 age spans need unsigned/checked arithmetic, including tolerated future timestamps, exact zero thresholds and maximal policy values.

Reviewer requested independent validation of public `age_at` invalid quality and trusted-quality-without-time cases, rather than relying on store normalization alone; Verified must be explicitly caller-supplied evidence, not verification performed by this model. Store updates must replace metadata without changing first-arrival order, evict deterministically at capacity, retain owned snapshots and preserve prior state on invalid/allocation failures. Copy/swap cost at the cap remains a target performance measurement gate.

## Executable evidence

C1 author freeze received and actual implementation inspected. The requested constructor/available-instant clarifications are present. Guarded signed addition avoids overflow before computing the sum; elapsed and evidence-age differences are checked before subtraction. Invalid or failing calculations precede the trivial value-state commit, preserving snapshot, preview offset and last monotonic cursor. Evidence status is validated even when otherwise stale; reset cannot fail and restores actual display. No I/O or shared/global mutable state was introduced.

A fresh warning-enabled ASan/UBSan build passed all **152 permanent clock checks** without sanitizer findings. The cases meaningfully exercise explicit zero-preview, forward/backward UTC jumps, missing/stale/future/Unknown/Unsynchronized evidence, reversed monotonic and invalid enum rollback, retained offset after failed advance, reset recovery, zero-age policy and signed counter extremes. A separate reviewer harness passed **74 wide-arithmetic oracle fixtures** for extreme UTC/offset additions and monotonic differences, using wider test-only arithmetic to derive expected valid/overflow/reversed outcomes. No remaining C1 source blocker found for this bounded model. Calendar formatting, absent wall-time representation, host synchronization, timer/alarm behavior and original unknown-time rendering are not supplied by it.

D1 frozen implementation validates identity, frequency, timestamp quality, location and bounded ASCII provenance before copying the aggregate. Callsign normalization is explicit ASCII and owns its result. Exact source/event identity is preserved; metadata replacement does not refresh arrival order. All mutation happens in a copied candidate vector, including eviction-key allocation and insertion, before a nonthrowing vector swap. Public age calculation independently rejects invalid quality/missing trusted timestamps; ordered unsigned subtraction represents the complete signed timestamp span without signed overflow. Verified remains caller-supplied, rather than a feed verification result.

A fresh ASan/UBSan standalone build passed **85 permanent spot checks**. Baseline warning flags passed; adding conversion warnings surfaced two implicit char-to-unsigned-char range-loop conversions in `spot_event.cpp` (lines 11 and 57), reported to the author as a warning-quality improvement, not a security defect. An independent temporary harness passed **518 checks**, injecting allocation failure at successive allocation points in update, below-capacity insertion and at-capacity eviction, comparing every stored field afterward; testing all 256 possible single identity bytes; and comparing signed-extreme ages/future tolerances against a wider-arithmetic oracle. All allocation-failure trials preserved the complete prior snapshot; no sanitizer finding occurred.

No remaining source blocker was found for these bounded original offline models. Neither source adds network, filesystem, radio, provider calls, dependencies, command execution, markup interpretation or global mutable production state. This source review does not clear future provider licensing or protocols. Printable, nonempty provenance is a structural label, not proof of location accuracy or reuse permission. Permanent tests exercise actual state and expected boundaries, rather than only successful construction. Maximum filled-store performance remains unmeasured; copy/swap is linear in stored events, and a 10,000-event capacity declaration does not establish Pi performance.

## Release gates and work queue

| Gate | Current evidence | Status |
|---|---|---|
| C1 header and source contracts | Independent arithmetic/rollback review; 152 permanent + 74 reviewer checks with sanitizers | Pass for offline model |
| D1 header and source contracts | Independent validation/mutation review; 85 permanent + 518 reviewer checks with sanitizers | Pass for offline model |
| Root build targets and whole-application checks | Await frozen Make/CMake integration | Pending |
| Original feature semantics | Dedup, clock-unavailable representation, filters, scroll state, alarms/countdown and source policy require separate implementations/evidence | Pending |
| Live provider adapters and permissions | No adapter, provider interaction or permission clearance in this increment | Blocked until separate review |
| Physical Pi performance / touch / accessibility | No target measurement or GUI integration in these models | Pending |

Approval is limited to integrating the reviewed offline model/test files into a private draft after root build checks. It is not original feature parity, security certification, legal clearance for external data, target performance or accessibility compliance.

## Reviewed source identity

SHA-256 at source gate:

```text
e6b73a8057bdbd798a898672e16cec2724213d005fa0d4a7e4b04c4a6107010a  src/domain/clock_state.hpp
c9618b3ba818d4baffbf57f9e1b1c386d91401b1e1694c61a37d039e3a485ae0  src/domain/clock_state.cpp
2269363271d511d6f2ded50167983500c54a6515c4f721cd7b177e1307ad51df  tests/clock_state_regression.cpp
75b31b27e5f1b4fbcc63a4070afb9c69dd5dabd34bf5281644dc2b0a89d0aee7  src/spots/spot_event.hpp
78a237393bedfb732989c7bbfce428f6c05ffa5abecfb6a6cd8fc492de15b4c3  src/spots/spot_event.cpp
19c7ad13d2c0733c280318d617a4e1749679e73a0de845056138ba8de3d6c368  src/spots/spot_store.hpp
cbd86eb57eeac7a2311d64751f385678829da0e39c4c469670993156aca57347  src/spots/spot_store.cpp
6a38b919e88bc88ba1fd32e28e322fdfe6786a7d9ecb2141e360858edc0fdb54  tests/spot_store_regression.cpp
```
