# Independent locator input audit

Status: **decoder and subsequent canonical-boundary encoder repair pass bounded independent review; original input-policy parity and linked persisted UI verification remain pending**. Reviewed 2026-10-03. Scope: `locator_input.hpp/.cpp`, `geometry.hpp/.cpp`, locator regressions and existing domain tests. Reviewer changed no production code, repository tests, build files or Git state.

## Evidence

A fresh warning-enabled C++17 AddressSanitizer/UndefinedBehaviorSanitizer build ran the actual regression: **79,416 checks, zero failures**, with no sanitizer findings. An additional independently authored temporary harness ran **1,540 checks, zero failures**: four manually derived southwest-corner fixtures and every possible byte in every position of a valid six-byte locator. The allowed character sets in that harness were specified independently of the implementation.

Independent coordinate fixtures included FN31pr (41 + 17/24 north, -74 + 15/12 east), QF56oc (-34 + 2/24 north, 150 + 14/12 east), PM95uq (35 + 16/24 north, 138 + 20/12 east), and AA00ac (-90 + 2/24 north, -180 east). Comparisons allowed 4e-14 degrees for floating representation and subtraction rounding. This host's `long double` does not provide a wider mantissa than `double`; these are independently derived formulas, not a claim of arbitrary-precision verification. An initially tighter 2e-14 tolerance rejected two valid fixtures by approximately one coordinate ULP; inspecting their numeric output justified the final tolerance without changing the decoder.

The permanent regression substantively covers all 324 two-letter fields and all 32,400 four-character squares at exact integral corners. Its six-character sampling covers every subsquare in representative fields/squares using interior re-encoding. That interior test deliberately does **not** prove corner re-encoding. Oversized strings, NUL, non-ASCII, punctuation, whitespace, incorrect letter/digit positions and unsupported lengths are rejected.

## Source findings

The decoder checks length before indexing or copying and accepts only 2, 4 or 6 bytes. Explicit ASCII ranges avoid locale-dependent case conversion and signed-character hazards. Field letters A–R, square digits 0–9, and subsquare letters A–X are independently bounded; lowercase letters are accepted only in letter positions. Invalid input throws `std::invalid_argument`. Integer subdivision arithmetic cannot overflow at these bounds, and valid corners stay within [-90, 90) latitude and [-180, 180) longitude. No filesystem or network operations occur.

The stated contract returns a southwest corner rather than a centre. Formula review and independent fixtures support that contract, subject to ordinary double approximation of fractional corners. Supported lengths remain a **provisional replacement input policy**: neither this audit nor these tests establish the final original HamClock's accepted input lengths or its chosen point within a grid cell.

## Linked-input blocker and bounded repair options

**Original integration blocker, high confidence, now repaired:** the former encoder assigned a decoded six-character corner to the adjacent cell. Independently reproduced before repair:

```text
location_from_maidenhead("AA00ac")
  latitude  = -89.916666666666671
  longitude = -180
maidenhead(result, 6) = "AA00ab"
```

The mathematical latitude is -90 + 2/24. Its double approximation and the encoder's subtraction/scaling produce a value just below the intended integral subsquare index. A linked grid input would therefore visibly change after conversion or persistence. The decoder itself follows its documented corner formula; integrating it without resolving this behavior would be misleading.

Before changing the encoder, explicitly choose its boundary semantics. A broad epsilon or unconditional upward nudge can classify actual coordinates immediately below a boundary as the next cell and should not be added merely to silence the round-trip test. A bounded repair can recognize coordinates equal to the canonical double reconstruction of a rational grid boundary, with an explicit documented tie rule; independently test the boundary and its immediate representable neighbours, negative/positive coordinates, square/field transitions, poles and antimeridian. Inherent ambiguity remains: the same double cannot reveal whether it originated from a locator or a measured coordinate.

If exact preservation of entered locator identity is required without changing coordinate classification, preserve canonical locator provenance explicitly through the edit/save/display model; this is a broader model decision and must avoid a stale locator after numeric coordinate edits. Changing the decoder to the cell centre is another coherent semantic choice only if confirmed against the intended behavior; it contradicts the current southwest-corner contract and is not an invisible numerical fix.

Required evidence was to reproduce the failure before repair, verify a specified finite lattice and nearest representable neighbours, preserve geographic endpoints, then verify the actual grid → coordinate → persisted settings → displayed grid flow. The numerical source gate is now covered below; the linked UI flow is not yet approved.

## Frozen boundary repair review

The encoder now searches the canonical representable boundaries directly: longitude `i/12 - 180`, latitude `i/24 - 90`, with 4,320 cells per axis. Its binary search keeps integral bounds, selects the greatest canonical boundary no greater than the coordinate, and terminates in at most 13 comparisons per axis. No offset addition to the input, epsilon or upward coordinate nudge is used. The documented tie rule assigns an exact canonical corner north/east; its immediate predecessor remains south/west. Latitude +90 selects the last northern cell; longitude +180 first wraps to -180. Validation still rejects nonfinite/out-of-range input before classification. The geographic calculations outside locator encoding are unchanged by inspection and their existing tests pass.

Fresh warning-enabled ASan/UBSan builds independently passed **155,519 boundary checks plus existing domain tests**, and **79,416 decoder checks**, without sanitizer findings. Permanent boundary tests exhaust each interior axis boundary and its immediate neighbours at precisions 2/4/6, verify endpoints and out-of-range endpoint neighbours, and pair every axis corner with nine opposite-axis transition points. Pairing is justified by the encoder's independent axis classification; this is not an exhaustive 18,662,400 paired-grid test.

The independent temporary harness was expanded to **11,548 checks, zero failures**: prior decoder fixtures/all-byte cases, 10,000 deterministic interior points compared with a separate conventional floor-based oracle, NaN/infinity on both axes, signed zero and immediate negative subnormal neighbours of zero. Interior sampling does not substitute for the exhaustive boundary cases. The original AA00ac fixture now encodes AA00ac at the identical decoded coordinates.

This policy consistently defines a representable floating grid and solves the demonstrated round trip. It is not a claim that every canonical double equals its mathematical rational corner, nor evidence of original HamClock numerical parity. A coordinate equal to a canonical representation is inherently indistinguishable from one derived by decoding a locator; the public contract now explicitly resolves that tie.

Frozen reviewed SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `src/domain/geometry.cpp` | `7c05ff7cc01606ca18abae3d6fa904db34db5c6ec7e550505f10b7b3003789c3` |
| `src/domain/geometry.hpp` | `1f0f705edfce81546f5f8ab13b1ae0a55d2181112501c8b5dcd391e350bdc8ed` |
| `tests/locator_boundary_regression.cpp` | `05e79c528cd0e8faaaca2384f2352f2d7e79f22b09501870f4913a39c4ef7f30` |
| `src/domain/locator_input.cpp` | `5293888787c422cbbf6843c6ac7ff77c68901ed4db4acbae091111b6db4c5028` |

## Disposition

No remaining source blocker found in the frozen decoder/boundary repair. Independent source review is frozen and supports proceeding to integration validation. The actual persisted linked UI flow still needs evidence. Original supported lengths and corner/centre parity remain research gates; no full product parity, Pi performance or UI approval is granted by these numerical checks.

## Build integration re-check

Root added the two locator executables to `make locator-check` and `make verify`, and as CTest targets. Both compile only domain sources; the boundary target correctly includes `src/domain/tests.cpp` for the existing geometry suite. Neither executable links Qt/SDL or adds a dependency. The existing top-level CMake project still requires Qt to configure its GUI targets; that requirement was not introduced by this increment. Header/source prerequisites in Make cover changes to the decoder and geometry API, and both CTest locator targets have a 15-second timeout.

Independent `make verify` passed all existing native/CLI/importer checks and both locator executables. To avoid the parent's concurrent build directory, the reviewer configured and built all targets from scratch in `/private/tmp/hc-locator-cmake-audit` using the installed Qt prefix. All **seven CTest targets passed**, including appearance, map, both dialog variants, dashboard and both locator regressions. No new build-integration blocker found. These executable checks do not substitute for root's planned actual loaded/persisted AA00ac GUI verification.
