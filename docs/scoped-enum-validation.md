# Scoped enum validation correction

October 11, 2026. Test/document-only correction; production formatter, domain contracts, native dialog and dashboard are unchanged from the tested private draft #31.

The previous test comment and audit rationale incorrectly described scoped SyncStatus as non-fixed and discouraged representable int values as undefined. A scoped enum without an explicit underlying type has fixed underlying int. Its representable unnamed values remain invalid for this product and must be rejected by the existing validator. The new standalone erratum explicitly supersedes that rationale while preserving historical report hashes and executed counts.

The formatter writer and independent auditor each checked primary C++ draft declarations/conversion rules. The corrected permanent regression statically asserts the actual underlying type and probes INT_MIN, -1, 255 and INT_MAX across source availability, preview, selected zone and analog/digital paths. Rejections preserve full supplied inputs and preferences. The production validator already handled these values correctly; this is broader evidence and corrected explanation, not a production behavior fix.

Author and independent strict C++17 warning/ASan/UBSan tests passed 2,236,456 permanent checks. The independent auditor also passed five supplemental wide-value probes with explicit enum sanitizer and a fresh isolated 26-target CTest suite in 8.67 seconds. Root local build, all 26 CTest targets and make verify passed. Exact corrected test and independent erratum hashes are recorded in the erratum/checkpoint. The corrected erratum and scoped validation document were independently cross-checked before private draft publication.

No new UI, dependency, clock/source conversion, provider, timezone mapping or hardware change is introduced. Native UI was not retested because its production source is unchanged; the previous final317 native observation remains scoped to that exact implementation. Hardware, accessibility, legal compliance, original feature parity and source accuracy are not established by an enum test correction.
