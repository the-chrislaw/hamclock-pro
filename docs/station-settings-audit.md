# Station settings independent audit

Review status: **locally verified for the bounded draft PR increment after fixes**. Scope: `src/platform/settings.hpp`, `settings.cpp`, `settings_tests.cpp`, CLI integration and executable regressions, reviewed on 2026-10-02. No production source was changed by this reviewer. This is a bounded code review, not a security certification or full product release approval.

## Re-review result

`make check integration-check` passed independently after the fixes. A freshly compiled AddressSanitizer/UndefinedBehaviorSanitizer harness also passed all existing settings tests and independent regressions: final symlink and FIFO saves are rejected while their entries remain intact; positive/negative smallest subnormals and the largest subnormal are rejected before an existing valid file changes; minimum positive/negative normal values, the next larger normal value and signed zero save and load exactly. The harness exited 0 and reported zero failures; no sanitizer findings appeared.

The previously confirmed unreadable-save defect is addressed by narrowing the persisted numeric domain to exclude subnormals before I/O. The final-destination replacement issue is addressed for stable trusted paths by rejecting existing nonregular destinations. The CLI bounds callsign/theme/numeric arguments, uses validated strings for JSON, catches errors with nonzero status and invokes no shell commands. No further blocking defect was found for this local draft increment.

This approval assumes the documented trusted, stable parent directory. The implementation does not protect hostile writable parents or prove crash durability. UI station editing, remote control, Pi testing and complete feature parity remain outside this increment. Retain the failure-injection and target gates below.

## Evidence

Compiled an independent temporary harness against the settings implementation, its tests and the geometry module with C++17, `-Wall -Wextra -Wpedantic`, AddressSanitizer and UndefinedBehaviorSanitizer on the local Mac. The existing settings test suite completed without reported failures. The harness exercised additional final-component symlink/FIFO saves, subnormal coordinate round-trip and strict numeric parsing. It exited 1 because an accepted value could not be loaded after saving. No sanitizer finding appeared.

The independent harness lived outside the repository in an isolated temporary directory. It did not use the user's settings or external services.

## Findings from the first review, now addressed

### Accepted coordinates can produce an unreadable saved file

`validate()` accepts every finite geographic coordinate, including `std::numeric_limits<double>::denorm_min()`. `save_settings()` writes this value with 17-digit precision and reports success. `load_settings()` subsequently throws `Invalid coordinate encoding` because the stream extraction rejects the subnormal representation on this platform. Reproduction: save `{de: {denorm_min(), 0}, dx: {90, 180}, callsign: "N0CALL", appearance: "dark"}`, then load the same path.

Guarantee that every value accepted for save is accepted on load. Either support the correctly represented subnormal values or validate a documented minimum magnitude before modifying any file. Add tests for positive/negative subnormals, minimum normal values and their immediate neighbors, plus preservation of an existing valid configuration on rejected saves. The best general guard is checking the serialized representation with the actual parser before committing it, alongside a consistent numeric-domain policy.

### Define and enforce filesystem destination policy

Independent tests confirmed saving to a final-component symlink replaces the symlink itself with a regular settings file. The referenced target remains intact. Saving to a FIFO also replaces the FIFO. Loading either is rejected. This asymmetry can destroy an unintended special directory entry selected as a settings path.

Reject nonregular existing destinations if that is the intended safety contract, and test that link/FIFO entries and referenced target bytes remain intact after rejection. Parent directory components are currently followed through ordinary pathname resolution. The temporary file creation, final rename and cleanup repeatedly resolve those components, so a stable trusted parent is a necessary assumption. State this explicitly for the local-only caller API or use directory descriptors and a deliberate parent-trust policy. Do not claim final `O_NOFOLLOW` protects all parent components. A pre-save `lstat` check alone does not resolve races in an attacker-writable parent.

## Passing checks and useful implementation properties

- Existing tests verify exact ordinary-double round trips, System/Light/Dark persistence, owner-only 0600 permissions, invalid-save preservation, duplicate/unknown/missing fields, version rejection, coordinate bounds, non-finite/hex input, callsign length, truncation, oversized data and missing-path errors.
- The reader bounds file size before reading and bounds accumulated bytes during reading. `O_NONBLOCK` plus descriptor inspection avoids blocking on a FIFO; final-component symlinks are not followed on load.
- `mkstemp` creates an exclusive temporary file; permissions are explicitly reduced to 0600; the writer handles short writes and interrupted writes, synchronizes the file and renames it into place. Failed rename cleanup is tested.
- Independent numeric checks rejected `1e-9999`, `1e9999`, `1.2.3` and `inf`; accepted `+1` and preserved negative zero. Callsign and appearance values are constrained data, with no shell execution.

## Limits and follow-up gates

The destination-directory rename is not synchronized, so successful save establishes atomic replacement but does not promise directory-entry durability across a crash. Concurrent writers have last-successful-rename behavior. Neither behavior is necessarily a blocker for this local draft increment if documented accurately.

Write/sync/close fault injection, hostile parent-directory races, disk exhaustion, target Linux behavior and owner/permission policies on load were not experimentally verified. These remain evidence gaps, not demonstrated exploits. Before a production release, add deterministic fault tests proving prior-file preservation and cleanup, verify the chosen parent/destination policy, and run the settings suite on Pi OS. No network service is included in this review.
