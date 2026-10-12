# Independent offline NOAA Kp parser audit

October11,2026. Reviewer owns only this report. Read actual frozen API and proposed original synthetic tests plus previously independently reviewed NOAA provider contract. No implementation/shared build/Git/dependency/UI/source edits by reviewer.

## Initial preimplementation header gate

The bounded offline API is coherent: raw valid civil timestamp stays zone-less with no UTC conversion; optional fields preserve supplied zero versus explicit null; finite Kp0..9 and nullable nonnegative int32 metadata; owned immutable result and null document on rejection; input order remains unchanged. Descriptive provenance is bounded ASCII and supplies neither identity authenticity nor data rights. Propagating allocation exceptions is explicit and acceptable provided no partial result escapes.

QtCore JSON needs lexical validation before object extraction because its usual object view does not preserve duplicate-key evidence. Header expressly rejects escaped-equivalent duplicate keys in every object, including ignored nested objects. Unknown fields are bounded syntactic input, not live measurement semantics.

Before implementation reviewer requested precise depth convention, decoded-key-byte definition, malformed UTF8/unpaired-surrogate policy and numeric underflow policy. The wording rejecting nonfinite/unrepresentable JSON must not silently permit nonzero exponents to become zero through Qt parsing, including ignored-field numbers. Exact boundary/member/key/depth and malformed escape/underflow tests are required. Concrete implementation gate awaits author header clarification/recheck; no source exists yet at initial inspection.

Initial header SHA256 `1f94ac86e9ae28401728c184b708f460468c7586b93d75343d4ae5b77fcaeb54`.

## Root native map validation documentation cross-check

Read docs/native-filled-map-validation.md. Observed computer-control actions are attributed to root rather than this reviewer; dimensions, timeout versus startup, mistaken click sequence, invalid/save/cancel fixture results and unresolved native/Pi/accessibility cases are distinguished. Independent offscreen/source results match prior frozen map audit. No unsupported full parity, physical Pi, legal/accessibility compliance or measured application startup claim found in that text. This document review did not repeat actual native UI operations.

## Corrected concrete API gate

Author corrected actual header before cpp: root container depth1, decoded UTF8 key bytes,128-byte numeric token cap, malformed UTF8/unpaired escaped UTF16 rejection, all numeric tokens finite including ignored fields, and nonzero mantissas underflowing to zero reject. Kp/unknown double precision limits are stated; nullable int32 metadata additionally requires lexical integer spelling and rejects fractional/exponent spellings rather than trusting rounded doubles. Corrected header SHA256 `d48e95a60706dae6dc10545132dacc720e5f697a5612acfc72681bff1e24e210`. **Concrete offline API gate approved before implementation.** Implementation and actual test evidence remain pending; no provider/live integration is approved.

## Provisional implementation finding

Before author final freeze, an independent strict sanitizer execution passed88 permanent checks but failed two of34 independent probes. Isolated installed Qt6.11.2 JSON tests confirmed `QJsonDocument::fromJson` accepts invalid JSON numeric spellings `.1` and `1.`. The original lexical preflight skipped leading-dot tokens or trusted QLocale conversion without strict JSON number grammar. Unknown fields must obey the same grammar as required measurements. Reviewer requested explicit optional-minus/integer/fraction/exponent token grammar and unexpected-start rejection, with expanded direct/nested malformed-number tests. Source was still undergoing author edits; intermediate formatter errors were not treated as frozen code evidence. **Implementation gate remains blocked until author correction and independent rerun.**

## Corrected implementation independently verified

**Earlier numeric grammar blocker resolved; bounded offline source/build gate passes.** Actual final source was re-read after freeze. Explicit RFC8259 number grammar now covers invalid dot/plus starts and every numeric token including ignored/nested fields. UTF8 decoding is stateless, raw BOM is rejected, escaped UTF16 pairs are checked before Qt extraction, decoded keys are checked for equivalence in every object, depth/member/key/token/byte/row caps remain finite. Provenance and civil-date schema validation precede immutable result publication. No partial document escapes late-row rejection; allocation exceptions propagate as documented. No mutation of caller buffers, I/O, network, cookies, credentials, logging, source execution, scheduling or dashboard activation.

Fresh strict C++17 warning-as-error ASan/UBSan build against installed QtCore alone: **104 permanent checks passed**. Separate original temporary harness: **48 independent probes passed**, including invalid .1/+1/1./leading zero/negative-leading-zero/incomplete exponent/literal suffix at root-row and nested unknown fields; overflow/nonzero underflow; exact zero extreme exponents; escaped-equivalent duplicate keys; unpaired/paired surrogates; exact/one-over member/key/depth caps; BOM/trailing content/NUL. Zero failures or sanitizer diagnostics. The reviewer independently isolated Qt's permissive numeric behavior before correction; final rejection is supplied by the original lexical grammar, not assumed from Qt.

Reviewed root CMake change: static hamclock-noaa-kp links existing QtCore only, warning flags, one regression target and15second CTest timeout. It is not linked into the dashboard or live feed. Fresh independent scratch configure/build `/private/tmp/hamclock-resumed-land-audit/kp-cmake` built all targets and **all15CTest tests passed**, zero failures,5.86seconds. No dependency installed and native app not launched in this offline increment; earlier native map acceptance remains separately documented.

Read actual research refinement: explicit null is retained as raw missing data, never promoted to a usable observation. This matches corrected nullable API and leaves semantic/live adapter gated. Research hash now `51906245c34f09b2b4dcd913f788eeb191100a63d98757e0d3d89103f71c6bf1`; original independently reviewed primary migration/rights evidence unchanged.

Source is an original bounded implementation using installed Qt APIs; no copied original HamClock implementation or captured provider bytes occur in permanent synthetic tests. This scoped review found no source-copying/provenance problem; it is not full legal certification. Qt redistribution obligations/version compatibility and hardware memory/performance remain separate product gates. Exact decimal precision beyond double is explicitly not preserved; metadata lexical integer checks avoid fractional rounding admission. Allocation failure injection was not exercised independently. No actual Pi3/Pi4/Qt6.4 execution, scientific measurement-quality validation, observation-interval/UTC certainty, real-source rights clearance, live freshness, full parity or accessibility claim follows.

Final hashes rechecked after execution:

- Header: `d48e95a60706dae6dc10545132dacc720e5f697a5612acfc72681bff1e24e210`
- Implementation: `cea59ed186337f86104813993b943a33ae330acaf38728f4efde8381206a433c`
- Permanent regression: `ddd591dc1d1480ba7a13ed1c6868c63d3c8fab040ac699503bcc49c5bcdfae72`
- CMake wiring: `08e3d04f2105778f32cfc438d51d7dccfc923e755bded96cdd4d868c5eaa01cc`

Exact next action: root may publish this bounded offline parser increment after its remaining verification/checkpoint steps. Before live use, independently resolve timestamp/interval/product-rights/quality semantics and transport; native replay presentation must be labelled and separately computer-tested.

Root-owned docs/noaa-kp-decoder-validation.md independently read after final tests: scoped offline raw decoder behavior,104+48 reviewer checks, independent whole-build reference, synthetic-only fixtures, explicit Qt6.11.2/Mac evidence and untested Qt6.4/Pi/live semantics agree with this report. No unsupported UI acceptance, rights, parity, hardware or compliance claim found. Research NK explicit-null refinement was rechecked as above. Audit report now frozen for root publication gate.
