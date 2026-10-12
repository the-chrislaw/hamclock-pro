# Endpoint, catalog and build integration review

Reviewed October 3, 2026. Status: endpoint increment and I01 offline catalog pass bounded source gates; corrected root build, all 10 CTests and Make verification pass. Only this report is reviewer-owned. No source/build/Git/assets/network changes were made by this audit.

## Independence and scope

This reviewer authored the original synthetic land importer baseline and the D1 spot model earlier. The independent endpoint review covers **root's new outside-boundary normalization increment**, not an independent re-audit of the reviewer's entire original importer. Original importer security review and D1 source approval were performed by `release_audit_plan`. I01 catalog and root build integration were not authored by this reviewer and receive independent review here. Research anchors: [land verification](../land-dataset-verification.md), [land plan](../land-polygon-implementation-plan.md), [widget peer review](widget-research-review.md), and [clock/spot independent audit](clock-and-spot-code-audit.md).

## Endpoint normalization evidence

Inspected `tools/import_land.py` `_geographic`, `_box` and point parsing, plus the added permanent endpoint fixtures. `_geographic` first rejects nonfinite input, then returns every in-range coordinate unchanged. Only an out-of-range excursion whose actual double difference from the endpoint is at most 1e-12 degrees is snapped to the same signed endpoint. Positive and negative longitude seam identities remain distinct; no modulo/wrap is introduced. Boxes reject source reversal before normalization and still require strict point containment afterward. No general epsilon is introduced for interior box containment or ring areas. Existing file/record/point/ring caps remain intact.

Reran all **31 permanent Python tests**, including malformed/nonfinite/oversize inputs, exact payload/ring offsets, endpoint envelope neighbors, closure/full-width pole retention, reversed bounds, strict interior containment, CLI publication preservation, bounded reads and nonregular files. All passed. Duplicate-name warning comes from the intentionally malformed ZIP fixture.

A separate temporary read-only harness passed **429 independent checks**: both endpoint limits/signs across 100 successive outside representable floats; exact binary retention of in-range boundaries/interior and signed zero; nonfinite rejection; the actual previously pinned archive's digest/count/polar-edge check. The pinned temporary ZIP was accepted with SHA-256 `1926c621afd6ac67c3f36639bb1236134a48d82226dc675d3e3df53d02d2a3de`, producing **5,143 points, 128 rings and 127 records**. The Antarctic full-width ±180 edge at −90 remained present. No generated repository asset or download was created.

No blocking endpoint defect found. Confidence high for the numeric normalization behavior and these bytes. This does not prove polygon topology, hole containment, absence of self-intersections, fill clipping, boundary alignment or visual correctness; source winding/order preservation alone is not a renderer certification. Normalization is an explicitly documented numeric policy, not evidence of original HamClock numeric behavior.

## I01 contract gate

Header inspected before implementation and approval sent to `instrument_catalog`: fixed replacement IDs for 24 known families, explicit nonexhaustive scope, all entries Unavailable, no fabricated readings. Three-slot model is deliberately single-choice and initially empty, without claiming original multiselect/rotation/default semantics. Invalid slot/unknown ID/duplicate assignments have explicit outcomes; duplicate rejection leaves all assignments intact rather than silently transferring. Catalog configuration does not activate a provider. Utility strip, rotation, persistence and countdown lifecycle remain separate contracts.

Frozen implementation inspected: constant bounded descriptors, no dynamic initialization work or I/O, linear lookup over 24 entries; slot/ID/duplicate validation precedes the sole trivial assignment. Labels and reasons are static plain text with meaningful unmet dependencies. Invalid queries avoid indexing. No providers, control, persistence or reading values are introduced.

Independent strict-warning C++17 ASan/UBSan rebuild passed **1,566 permanent checks**. Separate temporary sanitizer probes passed **199,349 checks**, exhaustively covering every uint16 ID, all invalid uint8 slots with prefilled state, invalid-slot precedence, unchanged unrelated assignments, and printable ASCII labels/reasons. No source blocker found for the stated offline catalog. No full parity, data permission, UI integration or accessibility claim follows from this bounded catalog.

## Root build integration finding

**Blocking integration finding, source dependency:** initial root Make/CMake spot-store regression targets omitted `src/domain/geometry.cpp`, while `normalize_event` calls its external `valid_location`. Independent scratch CMake configure/build reproduced `Undefined symbols ... hamclock::valid_location(hamclock::Location)` and exit 2. Finding sent to root without editing shared files. Standalone D1 author/reviewer builds had included this source, so those passing source checks remain valid; they did not validate root integration.

Scratch directory: `/private/tmp/hamclock-map-catalog-audit-20261003`. Environment: AppleClang 21, installed Qt base 6.11.2, CMake, Python 3.14.7 selected by CMake (permanent direct Python rerun used system Python 3.9). No installation occurred. Root added geometry.cpp to both targets and their Make prerequisite list, then re-froze. Independent fresh scratch reconfigure/build succeeded; **all 10 CTests passed**, including clock, spot, locator, land and existing offscreen Qt checks. **make verify passed** application self-tests, executable CLI regression, 31 coastline tests, 79,416 locator-input checks, 155,519 boundary checks plus existing domain tests, 31 land tests, 152 clock checks and 85 spot checks. CMake has explicit targets; Make source wildcard also compiled the newly audited catalog, without native integration. No remaining build blocker found. The initial failure is retained here so passing standalone checks are not mistaken for prior integration evidence. Do not infer Qt 6.4/Pi compatibility from this desktop environment.

## Exact next action

Return this frozen review to root for bounded publication decisions. Catalog Make/CMake permanent target wiring is a separate integrator increment if desired; catalog source approval does not make its widgets functional. No native UI changed in this increment; future consumers still require computer-control acceptance and physical Pi checks. Do not re-audit the reviewer-authored D1 as independent: its prior source approval remains the separate clock/spot audit.

Reviewed file hashes:

- `tools/import_land.py`: `280cddfa690805f04a5b5fada264321d719f9d598f32036110e058cffd441366`
- `tests/land_import_regression.py`: `99580619b0fb948e1ae124a228f51757408ca26ff33eae2a61a51a033c01e33e`
- `src/app/instrument_catalog.hpp`: `fb094f58f2a7ffcabfd1a31d7cb7926fd5ed9425f82548359371528e11134e00`
- `src/app/instrument_catalog.cpp`: `31672af4c910fe39d02ef7e272cb1951a8618792ff28eefdc2cfe5745ea24393`
- `tests/instrument_catalog_regression.cpp`: `0e36d942d7ac1165a0dd8a7318c29b9b754f3d2db8068845dbf1319def1b4ee9`
- `Makefile`: `4abf5b31d6ec1298e3d7c222b8bef706e786aa55eaeace2fff1068b7da7e104b`
- `CMakeLists.txt`: `79df2720cf85a1c9382c37feb5540516b9db7a7262d04575dcd88d145a4dfd4c`
