# Big Clock UTC civil adapter — CB2a validation

October 11, 2026. This original Qt Core adapter converts an explicitly injected optional POSIX UTC second to owned civil fields. It does not read or adjust the system clock, infer synchronization, select preview, convert DE time, obtain providers, persist settings or activate a native screen.

Missing source is distinct from epoch zero. The four-digit proleptic Gregorian replacement range is -62135596800 through 253402300799 seconds inclusive. Inputs outside it return OutOfRange before any Qt conversion, including INT64_MIN/MAX. In-range conversion explicitly uses UTC, checks Qt date/time validity, UTC identity and exact second round trip, then validates all civil fields with the reviewed pure formatter. Invalid conversion returns a typed unavailable result without fabricated midnight. The injected source remains owned and unchanged.

Compatibility policy uses the documented Qt::UTC TimeSpec overload below Qt 6.9 and the explicit QTimeZone::UTC overload on Qt 6.9 or newer. No global deprecation suppression, local-time default, system timezone lookup or new dependency is added. Primary Qt 6.4.3 declarations/QDoc and newer overload availability were independently reviewed; documentation is not target execution evidence.

## Executable and independent evidence

The author and independent auditor each passed 462 permanent checks under strict C++17 warnings and ASan/UBSan, repeated with ambient, America/Los_Angeles and Pacific/Auckland host timezone settings. An independent Python datetime oracle supplied 211 boundary and deterministic full-range snapshots, passed under strict sanitizers with Pacific/Kiritimati and America/Adak settings. This checks independence from host local timezone and independent conversion results.

Root configured/built locally, passed all 25 CTest targets and make verify. The independent auditor inspected the actual source and shared build diff, then freshly configured/built in an isolated directory and passed all 25 CTest targets in 6.38 seconds. The new target links only Qt Core and standalone presentation/preferences code; it is not linked into the dashboard runtime. No Make change is necessary for the Qt-specific adapter. The final scoped validation document was independently cross-checked before publication.

## Limits

Execution used installed Qt 6.11.2 on macOS. The pre-6.9 branch is reviewed structurally but not executed; Qt 6.4 and Pi 3/Pi 4 compatibility are unverified. InvalidConversion is a defensive branch, not fault-injected in these tests. No DE timezone mapping, DST alarm scheduling, leap-second labels, source accuracy/freshness, full original Big Clock behavior, accessibility/legal compliance or hardware support follows from these results. No UI changed; actual computer-control validation is required when this adapter is wired into a native view.
