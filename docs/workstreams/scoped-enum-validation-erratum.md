# Scoped enum validation erratum

October11,2026. Reviewer owns only this new erratum. Historical reports and production source remain frozen. Former formatter writer owns only its separately assigned regression-comment/coverage correction; root owns builds/Git.

## Correction

I incorrectly accepted and repeated the assertion that existing `enum class SyncStatus { Unknown, Synchronized, Unsynchronized }` has a non-fixed underlying type and that casting an int such as255 is undefined. **That assertion is wrong.** Scoped enums without an explicit enum-base have a fixed implicit int underlying type. All representable int values can be enum values, including values with no named enumerator; application validators must still reject unknown values. The default-int `enum class DeClockMode` follows the same rule. This does not apply the same rule indiscriminately to unscoped enums with non-fixed underlying types.

Independently reopened primary C++ working draft [dcl.enum paragraphs5/8](https://eel.is/c++draft/dcl.enum) and [expr.static.cast](https://eel.is/c++draft/expr.static.cast): fixed underlying values/conversion semantics support the correction. Existing production formatter compares against all three recognized SyncStatus values and rejects other int representations, so this rationale error is not a demonstrated production validation defect. Earlier executed counts and tests remain accurate historical evidence; the inaccurate comment/report explanation is superseded by this explicit erratum. No old report hashes are silently rewritten.

## Independent boundary evidence

Original temporary harness static_asserted underlying_type_t<SyncStatus> is int and compiled under strictC++17 -Wall -Wextra -Wpedantic -Werror ASan/UBSan plus explicit enum sanitizer. **Five unknown fixed-int cases** INT_MIN,-1,3,255,INT_MAX all safely reached the existing validator and threw invalid_argument. Zero failures. This is supplemental evidence, not retroactively added to historical permanent counts.

The former formatter writer independently checked the primary declarations and agrees. The completed permanent correction and independent rerun evidence are recorded in the correction gate below. Production enum/API/formatter behavior and native UI are unchanged; no compliance/parity/hardware claim follows. Root scoped validation and private draft review use those completed results, without rewriting historical reports.

## Independent correction gate

Former formatter writer independently reopened primarydcl.enum5/8 andexpr.static.cast8, inspected actualSyncStatus/DeClockMode declarations and agreed with this erratum's scopedfacts and historical-preservation limits. Writer's separately owned testdiff corrects only the rationale, adds underlying_type staticassert and fourwideintcases×16sourcepresence×preview×zone×face; unknownrepresentation rejection and fullinput/preference preservation are asserted. Productionheader/cpp and native UI unchanged.

Independently rebuilt/reran corrected permanent test with strictC++17 Werror ASan/UBSan plus explicitenumsanitizer: **2236456checks, zerofailures**. Independently repeated five supplementalINT_MIN/-1/3/255/INT_MAX probes: **5checks, zerofailures**. FreshuniqueisolatedCMakeconfigure/build passed **all26CTest targets in8.67seconds**. These are new correction-batch counts, not silent retroactive changes to2235432historicalcounts. No nativeUIrerun required for test/comment-only scope; prior native limits unchanged.

**Independent primaryfacts, exacttestdiff, strictsanitizer and fullbuild correction gates pass.** No productionvalidationdefect established or unsupportedfullrange-exhaustiveness/compliance/parity/hardware claim. CorrectedtestSHA256: `06e1058dc533790ec6e7e641f475719e32f031ffa4e15a61d7b6d7d84fdf019c`. Frozen productionformattercppSHA256: `4d015f102e69245158020c301ce08da063b2e4bc3ff097f6453a9557065ae0f3`.
