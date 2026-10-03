# Independent widget research review

Status: **corrected frozen research packets pass bounded critical-claim/coverage review; offline models supported, live-provider and unknown behavior gates remain blocked**. Reviewed 2026-10-03. Exclusive reviewer ownership: this file only. No code, shared documentation, build files or Git state changed. Scope: `instruments-and-space-weather.md`, `map-station-dx-and-clock.md` and coordinator ownership/verification board, not product security/legal certification.

## Baseline and coverage checklist

Independently opened the author's [HamClock 4.22 User Guide](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf), mirrored by G6NHU; its title and release notes identify 4.22 dated 2026-01-17. The mirror hosts the author's primary document; hosting alone does not establish permission to bundle its artwork/text. Use printed page numbers, which differ from PDF indices.

Manual coverage checkpoints: pane exclusivity, selection/rotation and scrolling (pp6–9,13); graph units/history and lower-region actions (pp7–9); propagation parameters and separate reliability/TOA maps (p9); satellite pair, age/pass/planner behavior (pp5,13); station full-precision editing/SW corners and map-specific selection/zoom (p10); time simulation/reset/unknown state, stopwatch/alarms/countdown/Big Clock (p11); projections/styles/overlays/info/RSS/night/cities (p12); persistent automatic/manual timezone behavior (p13). Remote/hardware effects of spot selection, Stop and visibility need explicit coverage (pp8,10). This is a checkpoint list, not a complete parity inventory. [Author manual](https://qsl.net/g6nhu/hamclock/hamclockkey-4.22.pdf).

Every writer feature row must distinguish observed original behavior, implemented replacement behavior, recommended design and unresolved evidence. A heading or pane name is not coverage of its menus, triggers, disabled states, configuration scope or side effects. Do not infer closed backend behavior or reuse grants from the client's credits.

## Independently sampled provider claims

| Provider | Verified primary evidence | Consequence / confidence |
| --- | --- | --- |
| NOAA/SWPC | [Data-access page](https://www.spaceweather.gov/content/data-access) lists JSON, text, images and products at services.swpc.noaa.gov. | High confidence that documented access paths exist; no live schema, exact product freshness or per-product permission is established by this index. |
| NOAA/NWS rights | [Disclaimer](https://www.weather.gov/disclaimer) permits NWS content subject to exceptions, attribution/endorsement restrictions; third-party material requires its provider's rights assessment. | High confidence that blanket permission for all imagery hosted by NOAA is unsafe. Derived charts must be identified as application output, not altered official products. |
| NASA media | [Media guidance](https://www.nasa.gov/nasa-brand-center/images-and-media/) generally permits informational NASA content, requires acknowledgement and distinguishes third-party material and endorsement/promotion restrictions. | High confidence that individual image origin/credit matters; no blanket third-party image or logo clearance. |
| CelesTrak | [GP formats](https://celestrak.org/NORAD/documentation/gp-data-formats.php) documents explicit format queries, OMM-related formats and current CSV default; [usage policy](https://celestrak.org/usage-policy.php) limits downloads per update and requires stopping/reporting non-200 responses. | High confidence: explicit format, approximately two-hour GP cache cadence and error stop are necessary. The current header reports six-digit catalogue IDs in use; TLE-only acquisition cannot cover those objects. These access policies alone are not a redistribution licence. |
| PSKReporter | [Developer guide](https://pskreporter.info/pskdev.html) documents retrieval fields, XML output, grid query mode and no-more-than-five-minute retrieval recommendation. | High confidence in this documented interface/pacing, not a guarantee of current service availability or blanket storage/redistribution permissions. Do not use the unrelated reporting cadence as retrieval evidence. |
| VOACAP | [Site](https://www.voacap.com/) describes free software but explicitly prohibits automated access to its services without prior agreement. | **High-confidence gate:** no automated web-service adapter approval from “free” software wording. Local engine licensing, input models and verified numerical output are separate paths. |

These pages were read as evidence; no telemetry query, account access, subscription or provider message was sent. Current provider policies differ from original 4.22-era assumptions and must not be silently represented as original behavior.

## Security and data-provenance acceptance checklist

These are reviewer-proposed engineering requirements, not claims from the manual:

- For every network dataset, record canonical HTTPS host/path, schema/version, units, observation and retrieval timestamps, quality flags, allowed missing values, forecast/observation distinction and cache policy. Show missing/stale/error states; do not replace missing data with zero, fabricated history or a promise of propagation.
- Bound compressed/uncompressed bytes, records, nesting, numeric ranges, image dimensions and allocation before expensive parsing/decoding. XML must prohibit external entities/network resolution; HTML/scripts must not execute; text labels must remain plain bounded text. Reject nonfinite values, invalid timestamps and ambiguous unit conversions.
- Use provider-specific pacing and stop rules, bounded timeouts/cancellation and shared request deduplication. A generic retry/backoff loop is insufficient when a provider requires stopping. Validate redirects/hosts before credential forwarding. Keep public data providers distinct from explicitly configured local devices/services.
- Separate tokens/API keys from distributable fixtures and diagnostics; redact URLs/headers/credentials and account identifiers. Persist secrets with an explicit permission/storage policy. Do not accept remote scripts, cluster-command injection, shell interpolation or arbitrary output paths.
- Treat station callsigns, location, spots and imported logs as potentially identifying data. Describe what leaves the device and why; no default diagnostic upload or unrequested forwarding. Radio tuning/rotator/PTT/GPIO actions need an independently tested boundary and visible intent, not a side effect of untrusted parsed data.
- Satellite inputs need catalogue identity, epoch/age, frame/time system, units, validity constraints and format support. Orbital algorithms/reference vectors and licences require their own gate; stale elements must not produce confident live tracking or hardware commands.
- For propagation, document model/version, assumptions, fixed/variable inputs, output meaning and independent reference fixtures. Reliability prediction, empirical spot activity and geometric paths are different data types; no renamed chart should blur those meanings.

## Legal and accessibility acceptance checklist

- Record per-source code, numeric data, imagery, geographic boundaries, fonts/icons and protocol specification rights separately. MIT covers our original code; it does not relicense third-party dependencies or grant data redistribution rights.
- Record licence/version/attribution, permitted runtime fetch, storage/bundling/republication, donation/commercial implications and restricted names/logos. Mark unresolved permission explicitly. A reachable API, free download, original author's credit or original project's negotiated permission is not our licence.
- Compare legitimate alternatives without copying archived implementation, brand assets, screenshots or bespoke provider graphics. Research facts can guide original code; asset ingestion needs provenance and review before it enters the repository.
- Each chart/map needs a readable summary, current numeric values/units/freshness and keyboard/touch equivalents for hover/click menus. Preserve logical focus and selected state; provide textual severity instead of colour alone. Test dark/light/System, scaling, focus contrast and reduced-motion behavior where relevant.
- Screen-reader behavior and physical Pi touchscreen usability need actual evidence. Framework accessibility metadata, nominal colour checks or a styled mockup do not establish ADA/WCAG compliance.

## Provisional coding-ticket disposition

Safe bounded preparation can proceed with original in-memory types and synthetic fixtures: provider-result states with timestamps/units, pure chart-axis/data validation, rotation/focus state machines, simulated clock state and bounded parser contracts. This approves neither a particular implementation nor real provider access. Each ticket needs exclusive file ownership, deterministic acceptance fixtures and a separate implementation audit.

Blocked until further evidence: VOACAP automated web access without agreement; reused KC2G/GIRO interpolated map assets without rights; external-provider redistribution assumptions; newly fetched satellite/hardware workflows without protocol/algorithm gates. Research review is timeboxed; unreviewed material must stay marked pending.

## Frozen document review and findings

Both author packets were read in full after their freezes. Their proposed APIs, limits and failure tests are clearly separated from original behavior; unavailable providers are not portrayed as implemented. File ownership proposals correctly leave shared source/build integration to root. No copied implementation or provider assets are proposed as a shortcut. Their two-path investigations are useful dependencies, not evidence that either path has already been proven.

Independent critical-claim samples additionally confirmed:

- [SILSO citation page](https://www.sidc.be/SILSO/citations) explicitly names CC BY-NC 4.0. The instrument packet correctly leaves donation/distribution interpretation unresolved rather than claiming that MIT or free downloading removes that restriction.
- [NTIA HF models](https://its.ntia.gov/software/high-frequency/high-frequency-propagation-models/) condition permitted reuse on retaining its notice. The instrument packet correctly restricts that conclusion to NTIA-developed software, not arbitrary ports, coefficients or online services.
- [SDO rules](https://sdo.gsfc.nasa.gov/data/rules.php) confirm imagery exceptions/credits and distinguish browse products from scientific use. The packet's chosen-asset review gate is supported.
- [GOES product](https://www.spaceweather.gov/products/goes-x-ray-flux) supports the two cited passbands, minute averages, primary/secondary mapping and calibration/eclipse gaps; [Kp product](https://www.spaceweather.gov/products/planetary-k-index) distinguishes three-hour estimates from minute chart refresh; [solar-wind product](https://www.spaceweather.gov/products/solar-wind) supports source switching and separate streams. These do not prove a specific live parser schema or upstream reuse chain.
- [IANA](https://www.iana.org/time-zones) currently lists 2026e; the geographic lookup dependency remains separately unresolved. [DXSpider](https://www.dxcluster.org/usermanual_en-4.html) and [Hamlib rigctld](https://hamlib.sourceforge.net/html/rigctld.1.html) support the packet's reception/posting and command/response distinctions. That is protocol evidence, not operator data permission or device approval.

Corrections sent directly to the exclusive writers:

| Finding | Requested correction | Gate |
| --- | --- | --- |
| Instrument catalog omits Countdown handoff | Add the manual's printed-p11 pane option to the clock handoff; keep exhaustive enum certification pending. | Catalog ticket must not erase this known family merely because its initial inventory inspected pp5–9. |
| Orbit wording may imply only a future limitation | CelesTrak's current header says six-digit IDs are already in use; retain legacy TLE input compatibility while planning explicit OMM-capable acquisition. | Wider current catalogue support needs an appropriate format and reviewed propagator. |
| GPSD time reliability too generic | The primary protocol recommends libgps; TPV UTC guidance requires consecutive 3D fixes/current leap-second knowledge. State those design/test dependencies explicitly. | Valid position and a timestamp string alone do not prove synchronized UTC. |
| Kyoto policy not independently retrieved | Reviewer received 502 from the cited policy page; retain author-verified but reviewer-unconfirmed status and block live Dst publication/access assumptions. | No independent permission conclusion granted. |

The first three corrections improve scope/implementation readiness; the fourth preserves an existing unresolved gate rather than asserting the author is wrong. No blanket factual audit of every endpoint, symbol, platform condition or original source enum is claimed. An immutable original revision and runtime observations remain missing evidence.

All four requested corrections are now incorporated in the exclusive writer files and independently re-read: Countdown catalog/clock handoff; explicit present orbit-ID limitation; libgps/UTC-quality details; and Kyoto reviewer-unconfirmed permission gate. This closes the research-document findings without clearing the underlying unknowns.

## Exact coding-readiness disposition

**Supported immediate offline batches, after root freezes their concrete API:** **C1** clock-state model and **D1** normalized spot/store model. They have noncolliding exclusive files and can be developed against original synthetic fixtures without provider calls, reused assets, hardware or shared UI/build writes. No model may claim complete original behavior or live observations from those fixtures.

**Supported bounded offline preparation:** **I01** provisional known-family catalog/selection including Countdown, with honest availability and no exhaustive-source-enum claim; **X1** nonexecuting selection intent after station-revision/capability contract freeze. **I02** can follow I01's frozen IDs using explicit injected freshness policy, rather than fabricated provider cadence. These are readiness recommendations, not implementation approvals: actual source and failure tests still require a different-agent audit before integration.

**Not immediately ready:** I06/I10/I12 depend on frozen data/catalog/widget contracts and native review; I08 needs original parameter enumeration or a separately stated restricted provisional contract before its ticket freezes. C2/C3 need rule backend/recurrence/DST decisions. M1 needs forward/inverse projection contract and evidence; M2 needs accepted polygon geometry/topology. D2/D3/D4 need documented dialect/grammar/protocol fixtures. A1 needs algorithm/reference/license evidence. These may be refined offline but their unverified interfaces are not approved simply because they make no network call.

**Live-provider/adapter gate retained:** **I03, I04, I05, I07, I09 and applicable I11 modules** remain blocked on their named transport/schema/rights/algorithm/hardware prerequisites. In particular, I09 may not automate voacap.com without agreement; I05 does not clear SILSO/Canadian rights; I11's Dst reuse is unconfirmed; SDO/other I07 assets need exact channel/credit review. This is not a conclusion that all possible alternatives are forbidden. It means the named implementation tickets have not yet met their dependency gates.

## Coordinator board and final limits

The root-owned [ownership/verification board](README.md) names one writer per research file and reserves shared integration/build edits to root. Its workflow requires frozen peer-reviewed evidence and independent actual-code audit, keeps blocked tickets blocked, and avoids simultaneous shared-file edits. No current ownership collision or premature live-ticket activation was found. File proposals are not active assignments; root must still recheck repository inventory when activating them. The board's future land endpoint-normalization limit is a proposed independently testable coding gate, not accepted real topology or renderer evidence.

The instrument packet contains **12 tickets I01–I12**; the map/station packet contains **12 tickets**. Review sampled critical primary claims, read both complete corrected documents and checked dependency/security boundaries; it did not verify every live payload, menu enum, original parser, default or platform condition. No code tests were run for these documentation-only changes. No provider was contacted, credential used, hardware operated or new asset bundled.

Frozen reviewed SHA-256:

| File | SHA-256 |
| --- | --- |
| `instruments-and-space-weather.md` | `244bee8583400ea46b0d7cd5691b684bc70f5f0ad663d8ca95a3d85d7944ab29` |
| `map-station-dx-and-clock.md` | `ed98e6b6107861878861a546c18b0e51c980292ad6acf57877c7f0316abcfdcf` |
| `README.md` | `44193e40ddf14f9437325d5890761da367f5eb26545e1647b6e17ca87c3742cc` |

The corrected packets support bounded research publication and the offline batches above. They do not establish full final-original parity, legal clearance, accessibility compliance, Pi performance or readiness to ship this week. Remaining source pins, permissions and user choices are explicit evidence gaps rather than assurances.
