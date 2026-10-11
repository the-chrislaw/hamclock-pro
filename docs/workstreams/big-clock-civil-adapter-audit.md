# Independent Big Clock UTC civil adapter audit

October11,2026. Reviewer owns only this new report; writer owns new Qt UTC adapter header/cpp/test; root owns builds/Git. CB1a formatter publication remains separate; this queued adapter has no UI, DE/location mapper, preview, synchronization inference, source freshness, scheduler or actual clock reads.

Initial header e9eeee27... read; requested explicit narrow version-compatible UTC call rather than newer-only API or global deprecation suppression. Read corrected header SHA25668a93d828d6160d068dad9fc251d9174d09ba222a22e3127b026aab30e8ec28f: **API gate accepted**. Optional injected POSIX instant separates absence from valid epochzero; source retained on failure; civil present iffReady; explicit MissingSource/OutOfRange/InvalidConversion/Ready. Inclusive proleptic four-digit product bounds -62135596800..253402300799 independently verified by datetime subtraction, prechecked before anyQt conversion. Supplied seconds are not normalized; datetime/date/time validity and complete CB1a fields checked. No partial shared state; Qt allocations may throw.

[Primary current Qt QTimeZone documentation](https://doc.qt.io/qt-6/qtimezone.html#QTimeZone-1) independently reopened: UTC lightweight initializer and constructor since6.5, including representation without timezone feature. Installed6.11 qdatetime.h marks TimeSpec overload deprecated since6.9. Independently read hash-pinned official6.4.3 declarations/QDoc in civil research, including explicitQt::UTC overload and documented extreme-seconds undefined precondition. Approved compile-time branch Qt<6.9 explicitQt::UTC; Qt>=6.9 equivalent lightweightQTimeZone::UTC. No global warning suppression/local/default/backend call. Baseline actualQt6.4build/Pi remains untested; browser raw6.5 fetch failure is not evidence of tagged6.5 execution.

Actual source/tests and independently rebuilt strictsanitizer/full-root-build/validation gate pending. Required probes include exact productbounds and adjacent/rejectedINT64 extremes, epochzero/-1, century/leapday/goldenvalues, deterministicUTC output acrosshostzone changes, retained source/ownedresult, no clock/IO. No DST/alarm/legal/backend/locationmapper/hardware/compliance/parity promotion.

## Actual source and independent UTC probes

Inspected final source: absence and inclusive-product-range rejection return before every Qt conversion. Qt>=6.9 uses explicit lightweightUTC; baseline branch uses explicitQt::UTC. Both preserve input source; no LocalTime/default/backend or global warning suppression. After conversion, datetime/date/time validity, UTCtimeSpec and exact seconds roundtrip precede complete publiccivilvalidation; only then ownedcivil Ready. Validator invalid_argument becomes typed InvalidConversion; preparation/Qt exceptions are not mislabeled rollback. No mutable state or actualclock/IO.

Independently strict C++17 -Wall -Wextra -Wpedantic -Werror ASan/UBSan compiled/reran **462permanentchecks, zero failures**, separately ambientTZ,America/Los_Angeles,Pacific/Auckland. Permanent tests include12literal goldens,432calendar-month boundaries, absent/epochzero/negative/productbounds/adjacent/INT64extremes and ownedvalues. Separate original harness generated **211golden civil snapshots** using Python datetime+timedelta (11bounds/epoch/day edges+200seeded full-range values), independently compared every source/status/civil field, passing strictsanitizer separately in Pacific/Kiritimati and America/Adak. These are211unique probes repeated in twoTZ environments, not422uniqueprobes.

No-Qt-call on out-of-range input is proven by actual early-return sourcecontrolflow plus dynamic typedclassification; no instrumentedcallcounter is claimed. Qt<6.9 baseline branch remains structural/unexecuted; InvalidConversion defensivebranch is not fault-injected. No arbitrary undefined extremeQtconversion was performed. ActualQt6.4/Pi/runtime distribution/sourcequality/DE/backend/DST/legal/accessibility/parity gates remain separate. Rootsharedbuild/full25suite/validationdocument gate pending.

Frozen actualsource hashes:

- src/qt/big_clock_civil_adapter.hpp: `68a93d828d6160d068dad9fc251d9174d09ba222a22e3127b026aab30e8ec28f`
- src/qt/big_clock_civil_adapter.cpp: `01fcf648d2303b277f7659d36536e22f7f321df94e977091b39fb315f9705fbc`
- tests/qt_big_clock_civil_adapter_regression.cpp: `393263f03ec930baace485b512abd2a2b579167d514f5b1b99a587f22517a926`

## Independent shared-build gate

Reviewed exact root new CMake diff: standalone BUILD_TESTING QtCore-only adapter regression with complete pureformatter/preference sources, warning flags, TZ=UTC and15secondtimeout; no runtime/dashboard link, UI, new dependency or Make edit. New adapter is in src/qt and outside existing experimental Make wildcard. No need repeat unchanged previously verified Make solely for this independent test wiring; actual adapter executable is tested through strictcompile and CTest.

Fresh unique isolated CMakeconfigure/build passed **all25CTest targets in6.38seconds**. Earlier strict462permanent reruns and211uniquePython-datetime oracle probes across differingTZ environments remain finalsource evidence. Qt6.11.2 current branch tested; tagged6.4declarations/oldbranch source reviewed, not executed. InvalidConversion defensivebranch not fault-injected; no impossibleQt extreme probe. Root scopedvalidationdocument/hash gate pending.

Reviewed CMake hash: `5c46dcf9f07e1f230cc61b8c03897dc7b9dbb5e19961d331f4d4fc0c93b14d11`.

## Final scoped validation gate

Independently reread final rootvalidation, including exact462permanentchecks repeatedthreeTZ,211uniquePythonoracle snapshots repeatedtwoTZ, independentfresh25CTest6.38seconds and root25CTest/makeverify distinction. Pre-callbounds sourceproof, retained injectedsource, explicitUTC/roundtrip/publicvalidation and version-specific branch are accurate. Documentation/tag review is separate from executed installedQt6.11.2. ActualQt6.4/Pi remains untested; InvalidConversion defensivebranch notfault-injected; noDE/scheduler/provider/sourcequality/UI/compliance/parity promotion. No runtime/dashboard linkage and noMakeedit. Frozen source/build hashes unchanged.

**Bounded CB2a API/source, independent strictsanitizer/oracle/fullbuild and scopedvalidation-document gates pass.** Ready for root private draft review increment within existing authorization. Nativeview integration/computerQA, baseline/hardware and runtime distribution/dependency/rights gates remain separate; no merge/release/public/hardware/compliance/fullparity readiness. Rootvalidation SHA256: `0ddddc7bc83b3972f1611f17dd7ffdc1578b326dafda904077f07168afb05767`.
