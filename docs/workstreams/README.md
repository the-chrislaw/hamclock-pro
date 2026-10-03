# Widget workstreams and verification gates

The approved home-screen mockup defines composition, not the full behavior inventory. Feature parity is measured against final-original HamClock4.22. Every pane's selectable contents, menus, settings, inputs, outputs, states and data/service permissions must be accounted for before a completion claim.

## Active exclusive ownership

| Agent | Exclusive research file | Scope | Independent verifier |
| --- | --- | --- | --- |
| station_settings | instruments-and-space-weather.md | All selectable instrument panes, Sun, propagation and space-weather utility | release_audit_plan |
| dashboard_audit | map-station-dx-and-clock.md | Map, DE/DX, clocks/timezones, cluster, satellite/control interactions | release_audit_plan |
| release_audit_plan | widget-research-review.md | Independently verify primary evidence, coverage, quality/security/legal gates | root checks disposition and contradictions |
| root | README.md and ../overnight-checkpoint.md | Ownership, dependency queue and integration decisions | independent reviewer of each code/build increment |

No research writer edits another writer's file or production source. Reviewers send findings to writers; the writer owns corrections. A frozen reviewed snapshot is required before coding tickets are marked ready. Any changed snapshot needs its relevant findings rechecked. Reviews must state pending/unknown facts rather than certify missing evidence.

## Required ticket contents

Each ticket names the original behavior/manual page and primary evidence; input/output API and configuration dependencies; exact exclusive coding files; source/license/service-policy provenance; bounds, privilege and credential handling; loading/empty/stale/error/offline behavior; and meaningful numerical/unit, executable, native interaction and hardware acceptance gates. Unknown choices include two plausible paths and the next two implementation/research steps. Original code only; source data licences remain separate from MIT application code.

## Workflow

1. Researcher freezes evidence/specification/tickets with confidence and unknowns.
2. Another agent verifies important claims against primary sources and checks behavior coverage/security/licensing. Blocked tickets stay blocked; unaffected reviewed tickets can proceed.
3. Root assigns a bounded coding batch with exclusive source/test ownership and dependencies. No concurrent writes to shared build/integration files.
4. An independent agent audits the actual frozen code and reruns meaningful tests. Root builds locally, executes applicable regression checks and computer-tests UI changes. Source/widget tests do not replace UI or Pi validation.
5. Fix/recheck findings before a bounded private draft PR. Root attaches it and records exact next action. No merge, public release or unsupported parity/compliance/hardware claims.

## Current implementation queue

- Audited synthetic land importer: publication in progress, separate from dataset/fill implementation.
- Real land archive verified as4.1.0 in temporary storage; strict parser rejects tiny endpoint overshoots. Next coding batch needs explicit at-most1e-12 outside-boundary snapping, interior preservation, beyond-limit/nonfinite rejection and independent tests/provenance review before bundling data.
- Real land fill consumes reviewed rings/holes/records with explicit antimeridian/polar cases; do not fill coastline fragments.
- Widget coding tickets will be selected from the reviewed research, not invented from the mockup's example charts. Data adapter/network/model/provider decisions remain unapproved until source-specific evidence is reviewed.

Actual release gates include Pi4 seven-inch touch first, Pi3 performance, on-screen keyboard, browser control, assistive technology and complete4.22 behavior evidence. Donations/open-source intent alone does not settle upstream data/service permissions.

## Coordinator's independent spot checks

Root separately retrieved the current [VOACAP primary site](https://www.voacap.com/) and confirmed automated online-service access requires prior agreement; free standalone software does not confer online API permission. NASA's [media guidelines](https://www.nasa.gov/nasa-brand-center/images-and-media/) and [NOAA disclaimer](https://www.noaa.gov/disclaimer) were separately retrieved for reviewer crosschecking; source-specific material/third-party exceptions still require endpoint-level evidence. These checks support a blocked online-service ticket, not a conclusion that every candidate dataset is forbidden or cleared. SILSO edition/licence, orbit-format policy and writer coverage remain reviewer tasks.
