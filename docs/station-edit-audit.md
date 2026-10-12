# Independent station draft model audit

Status: **locally verified for this bounded model increment**. Reviewed 2026-10-03 after `station_edit.hpp`, `station_edit.cpp` and tests were frozen and root integrated the test entry point. No production code or tests were edited by the independent reviewer. The native editor UI is not connected; this report makes no widget or display claim.

## Evidence

Fresh `make verify` passed the integrated native self-tests, executable regressions and 31 importer methods. A separate C++17 harness ran the frozen edit tests and additional independent cases with warnings, AddressSanitizer and UndefinedBehaviorSanitizer. It exited zero, reported zero failures and produced no sanitizer findings.

Additional fixtures checked negative/minimum/maximum integer casts to the field enum; embedded NUL, comma decimal, malformed signs and repeated decimal punctuation; encoded extreme underflow `1e-9999`; negative subnormal text `-1e-320`; negative zero; and locale independence under a custom global comma-decimal locale. On this local standard library, extreme underflow was rejected, rather than silently accepted as zero. Minimum-normal baseline values round-tripped exactly. Pi OS standard-library behavior still requires target verification.

Every rejected validation case retained the original draft text, and cancellation restored the clean baseline, including negative zero. Altering a returned settings copy did not change the editor or caller's baseline. The test suite separately covers each field's exact storage cap and over-limit rejection without mutation, geographic bounds, invalid baseline construction, valid multi-field changes and appearance/callsign validation.

## Review findings

No blocking defect was found for this model increment. Malformed field enums are mapped through a guarded switch; writes reject unknown fields and reads throw. Draft storage caps are 16 bytes for callsign, six for appearance and 64 for each coordinate. Empty or incomplete text can be stored deliberately during editing; `validated()` is the explicit boundary that rejects it before returning settings.

Numeric encoding uses an ASCII whitelist and the classic locale, rejects trailing characters, and passes returned settings through shared presentation/domain validation. The constructor validates the baseline before snapshotting it; snapshots own their strings rather than referencing the caller. Validation does not commit or replace the baseline. Cancel restores all six strings, and dirty state compares draft text with that original text; semantically equivalent alternate number spellings may therefore remain dirty, which is consistent with a text draft.

The reviewed model contains no filesystem, network, shell or device operation. Its shared validation path computes geometry and presentation only. A future GUI must call this explicit validation boundary and implement any save/commit step separately; calling `set()` alone is not acceptance of a complete valid station.

## Remaining gates

Native widgets, field navigation, error announcements, touch targets, focus restoration, submit/cancel wiring, saving valid changes and display-theme behavior remain unimplemented by this model. Target Pi numeric parsing and performance are unverified. Duplicated validation policies across persistence, presentation and editing should remain synchronized when changing accepted settings. This bounded review does not establish complete accessibility, security certification or original HamClock feature parity.
