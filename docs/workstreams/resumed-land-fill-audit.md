# Resumed LF1 independent audit

October 11, 2026. Reviewer owns only this new report. Read the LF1 contract, prior review and actual frozen header/implementation/regression; no applicable AGENTS.md was found in project, workspace parent or Codex parent. No source, shared build, Git, dependencies, network, app, station or radio operation changed. Temporary executables/harnesses are under `/private/tmp/hamclock-resumed-land-audit`.

## Disposition

**Not ready for integration at initial audit:** implementation is a complete bounded helper, but its permanent synthetic executable fails one fixture. All strict builds succeeded; the permanent 65-check run reports one failure, `full width Antarctica survives`. Its rectangle has a 360-degree top edge at latitude -80, which contradicts the header's supported-seam rule (only exact full-width edges at +/-90 poles). The helper correctly returns UnsupportedSeam for that rectangle. An independently constructed stepped top boundary (-180,-90,0,90,180 at -80) with the exact full-width bottom closure passes containment and raster checks. Resolve the fixture/contract mismatch through the author and rerun permanent tests; do not silently permit arbitrary nonpolar long edges.

Actual pinned-source testing now passes. Root restored the exact previously reviewed archive and generated header into temporary storage; the reviewer did not acquire or generate them. No real asset is approved for repository inclusion by this report, and no native widget integration is approved until its separate gates.

## Actual checks

- Strict C++17 `-Wall -Wextra -Wpedantic -Werror` with ASan/UBSan, installed QtCore/QtGui only: permanent regression builds and executes offscreen. **65 checks, 1 failing polar fixture** described above; no sanitizer diagnostic.
- Independent temporary adversarial executable: **22 checks, zero failures**, including stepped south/north polar closures, ordinary seam rejection, nonpolar full-width rejection, strict nextafter exterior coordinates/NaN/Inf, huge record/ring offsets/counts, exact microtriangle coordinates, attached contact elements, and polar QImage samples at widths 180/720/1440. No sanitizer diagnostic. This is independent evidence, not a permanent project test.
- Pinned archive SHA-256 `1926c621afd6ac67c3f36639bb1236134a48d82226dc675d3e3df53d02d2a3de`; temporary generated header SHA-256 `cc2d3904939d2cfe258660b014cdfd19c106a8f65c18dbd034cd13f0f57f72dc` match prior evidence.
- Full 5,143-point/128-ring/127-record pinned source admits exactly 127 paths. Independent long-double winding computation reproduces London/Tokyo/Los Angeles land, Atlantic/Pacific ocean, Caspian candidate hole and Antarctica. Qt containment agrees with each independently computed winding result.
- Full-source QImage raster samples at 720x360, 1440x720 and 2880x1440 verify safe Atlantic/Pacific/hole ocean and Antarctic interior pixels. **31 pinned-source checks, zero failures**, strict sanitizer build, no sanitizer diagnostic.
- Record79 preserves all 14 QPainterPath elements. Its tiny triangle barycenter has triangle winding +1 and continuing main-outline winding -1, total zero: **ocean under the supplied nonzero winding model**, rather than an assumed land microtriangle. Qt contains and an enlarged local QImage render (scale 1e12) agree with this independently computed cancellation. An initial reviewer assumption that its barycenter must be land was falsified; that experimental expectation was corrected, not the source or renderer.

## Source review

Offsets are checked before pointer arithmetic/dereference; contiguous coverage induction keeps subtraction safe and prevents count addition overflow. Caps precede memory access. Coordinates are finite and strictly bounded; +180/-180 side identity is preserved. Closed rings require three distinct points using linear scanning and nonzero accumulated planar signed area. All points/order reach per-record WindingFill construction, with no shortest-edge wrapping, simplification, topology repair, provider or runtime file loader. Input views require caller-valid backing storage: this cannot validate arbitrary pointers. All-or-nothing output catches std::bad_alloc during construction; allocation failure injection was not independently exercised here. QPainterPath may normalize redundant coincident path operations internally, so literal raw-point storage is not supplied by this API.

This reviewer did not find a memory-safety or source-copying issue in the bounded helper. That is a scoped code audit, not legal certification. Qt distribution obligations, Natural Earth provenance, cross-record topology, performance and native contrast remain separate gates. Long-double area/winding calculations are numeric diagnostics, not an exact exhaustive topology oracle. Qt 6.4 and actual Pi3/Pi4 were not executed; QImage evidence does not substitute for computer-controlled native UI validation.

## Frozen hashes at initial audit

- `src/qt/land_fill_path.hpp`: `59d4f344c668018ee90feaa286f92eef29230153325d01d5dc9221412fee2ea1`
- `src/qt/land_fill_path.cpp`: `4f2a03e0a5245cf08d9bfcb50e191edbc2b4b87ec1e76fbb1eecf7ed55e06e32`
- `tests/land_fill_regression.cpp`: `54251afc26fa130fc7e6ec0ce00c555037972e8a386256462d2f366060e9a72f`

Exact next action: author corrects only the inconsistent polar fixture while preserving strict seam policy, then reviewer reruns and rehashes. Root may integrate the passing synthetic helper into its build after that check; LF2 real asset/provenance and LF3 native fill remain distinct follow-on gates.

## Author correction independently verified

The earlier blocker is **resolved** after actual rerun, not waived. Author changed only the permanent regression: the Antarctic top now contains an explicit Greenwich intermediate vertex, leaving each nonpolar edge at 180 degrees; the formerly incompatible rectangle remains a permanent UnsupportedSeam rejection test. Production header/implementation hashes are unchanged.

Corrected permanent executable freshly rebuilt with strict C++17 warning-as-error and ASan/UBSan flags against installed QtCore/QtGui: **93 checks, zero failures**, offscreen execution and no sanitizer diagnostics. The reviewer reran both independent executables: **22 adversarial checks and 31 full pinned-source/raster checks, zero failures**, with no sanitizer diagnostics. Exact record79 fixture data in the permanent test was compared independently to the raw pinned SHP: all 14 coordinate pairs match byte-decoded doubles. Raw SHP hash matches `8689e6932b8e370e2ca4587cf3ba21e460b1235db37b6ed3c172c35b4a6088de`. Separate exact binary-rational Fraction predicates reproduced permanent fixture winding -1 at (-132,53.5) and zero at (-134,53.5). Its local raster and clipping checks are bounded evidence for the contact outline, distinct from the reviewer's highly enlarged microtriangle cancellation probe and full-layer checks.

Corrected frozen regression SHA-256: `55eca82e63112979d2a779190fcb382f4c1491f9f2ddb8bbcb30dd34b9a0550e`. Header and implementation remain the initial hashes above.

**LF1 helper/source gate passes for root build integration and continued bounded offline experiments.** This does not approve LF2 repository land asset/provenance, LF3 native world-map integration, general topology, Qt6.4 compatibility, Pi performance, full HamClock parity or accessibility/legal certification. Exact next root action: add helper and permanent regression to the appropriate build targets and rerun project regressions; real asset/provenance and native computer-control remain separate queued increments.

## Root build wiring independently verified

Reviewed the frozen CMake diff: one BUILD_TESTING-only executable from the helper and permanent regression, links existing QtCore/QtGui, warning flags, offscreen test environment and 15-second timeout. No production widget dependency, asset, runtime or provider behavior changed. Independently configured a new scratch tree `/private/tmp/hamclock-resumed-land-audit/cmake` with installed Qtbase, built all targets with two workers, and ran all **13 CTest tests: 13 passed, zero failed**, including land-fill and all existing map/dashboard/editor/model checks. Configuration's optional Vulkan-header message was nonfatal; no new dependency was installed. Native app was built but not launched or computer-controlled in this test-only increment. Source hashes were rechecked after execution and match the corrected freeze. The build integration gate passes for this bounded test helper increment.
