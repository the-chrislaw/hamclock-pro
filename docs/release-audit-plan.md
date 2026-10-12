# Independent audit queue and release gates

This queue applies to draft changes for an original implementation of the final original HamClock 4.22 feature set. A passing draft review permits integration review; it does not authorize merging, publishing or claiming full parity. Pi 4 with a seven-inch touchscreen is the first user test target; Pi 3 remains required.

## Status vocabulary and evidence

Use `queued`, `implementing`, `ready for independent audit`, `changes requested`, `locally verified`, `target verified`, or `blocked on named evidence`. Record the reviewed commit, commands and exit results, fixture origin, reviewer, and remaining limitations. A developer's self-test result is useful evidence but is not independent verification. Reviewers must exercise failure paths, not just repeat happy-path tests.

| Queue | Present evidence | Gate to pass next |
| --- | --- | --- |
| Geometry and coordinate CLI | Existing source inspected; `make check` independently passed locally on 2026-10-02. Analytic equator, pole distance, dateline, coincident and antipodal fixtures exist. | Add independent CLI failure/output checks; specify spherical distance and pole-bearing conventions. |
| Persisted station settings | Not audited in this review; implementation assigned separately. | Complete settings matrix below on actual source, including filesystem attacks and preservation checks. |
| Real geographic map | Existing native code contains explicitly illustrative schematic polygons. | Obtain licensed geographic data and numerical projection evidence; integrate actual asset and run target measurements. |
| Accepted visual design | HTML design study exists; native renderer still uses rejected experimental style. | Native or selected production UI must pass input, theme, reflow and assistive-technology checks. |
| Full original feature parity | Complete, independently validated 4.22 inventory and working parity evidence are not present in the reviewed source. | Trace every inventory item to code, observable behavior and verification; unresolved items prevent a full-replacement release. |
| Pi release readiness | Local desktop test result only. | Pi 3 and Pi 4 builds, hardware input/display, measured resource usage, install/uninstall and recovery evidence. |

## Settings: mandatory independent tests

Use an isolated temporary directory and compare original file bytes and permissions before and after each rejected operation. Do not touch a real user's settings in tests.

- Exercise valid bounds, boundary-adjacent values and out-of-range latitude/longitude; reject NaN, infinity, exponent overflow, hex numbers, empty/missing values, trailing text and oversized input before unbounded allocation. Verify the documented policy for negative zero and underflow.
- Exercise missing, duplicate and unknown keys; wrong types; unsupported schema versions; truncated input; oversized files; embedded NUL; invalid encoding if strings accept text. Require an explicit duplicate-key policy, preferably rejection. Never silently merge ambiguous coordinates.
- Reject unsafe file destinations as specified by the persistence contract: final-component symlinks, directories, devices, FIFO and untrusted ownership. Examine parent-directory trust and races: a path check followed by an ordinary open is insufficient. Use descriptor-based or no-follow operations appropriate to each supported OS; test replacement of a link between checks where practical.
- New private configuration and temporary files must have owner-only access (0600). Check behavior under both permissive and restrictive umasks. An existing configuration must not become more readable during replacement.
- Provoke temporary-create, write, sync, close and rename failures using a test seam or documented fault harness. Existing valid file bytes must survive any failure before the atomic commit. A partial write must never be treated as success. Confirm no abandoned sensitive temporary files and no clobbering of an unrelated existing temporary file.
- Successful replacement must be atomic within the destination filesystem. Record whether file sync and parent-directory sync are provided, and distinguish atomic replacement from crash durability. Error messages must identify the operation without exposing credentials or arbitrary file contents.
- Restart after a save and compare all settings; handle a missing file with documented defaults and corrupted data with an explicit error. Exercise simultaneous writers and document last-writer or lock behavior.

## Geometry and real map: mandatory independent tests

- Verify analytic distances (one equatorial degree, quarter and half circumference), cardinal initial bearings, longitude wrapping, both poles, dateline crossings, coincidence and exact/near antipodes. Bearings at geographic poles require an explicit convention; undefined cases must not fabricate a heading. Verify distance symmetry, short plus long circumference and finite/nonnegative results across a deterministic global sample.
- Verify Maidenhead fixtures from independently derived expected cells, all supported precisions, field/square/subsquare boundaries and exact endpoints. Expected values must not simply call the production conversion again.
- Record map dataset publisher, exact version/download URL, license terms, redistribution/attribution requirements, local asset hash, transformation process and notices. Do not infer permission from public downloadability or apply the application's MIT license to third-party data.
- Specify each projection's formula, center/extent, unit conventions and supported overlays. Compare known equator/meridian/pole points and nontrivial fixtures to an independent reference. Check forward/inverse round trips where supported, dateline clipping, polar behavior and paths that cross the seam; no world-spanning stray segment.
- Verify station pins, locator boundaries, gray line and short/long paths share the same geographic transform. Validate each overlay independently; a plausible coastline is not proof that the overlay is correct.
- Exercise malformed or absent asset handling with bounded parsing, finite coordinate validation, maximum feature/vertex limits and a visible error rather than silent substitution with demo data.
- On Pi 3 and Pi 4, record OS/build/display, data size, cold-start time, peak resident memory, idle and interactive CPU, redraw timing, touch latency and any thermal throttling. Establish an explicit budget from those measurements before claiming suitability. Cache static geography and measure updates separately from full redraws.

## UI and remote-control gates

- System/Light/Dark: default System; follow changes while running; explicit override remains stable. Test all states in both light and dark OS preferences and after restart if preferences persist.
- Keyboard access to every control, visible focus, ordered navigation, Escape handling, dialog focus containment and restoration; no action accessible only by a precise map click. Test seven-inch 800 × 480 layout, text enlargement and zoom/reflow without lost controls. Touch mode should provide 44 px targets; record the actual measured target sizes.
- Check text contrast, control-state contrast and information conveyed by color with independent measurements. Charts/maps require meaningful alternatives and labels. Validate the production UI's actual accessibility tree and a target screen reader; HTML semantics do not certify an SDL canvas.
- Local browser control remains a separate security-sensitive feature: authenticate state-changing requests; restrict interface exposure deliberately; test CSRF/origin checks, malformed payloads, authorization failures and configuration updates. SSH administration does not establish browser-control support. Never add a privileged command endpoint as a shortcut.

## Current source observations

No network service, credential processing or shell command execution was found in the reviewed native foundation. The coordinate CLI bounds text before allocation and rejects non-finite input. Existing domain tests cover useful numerical edge cases and passed locally.

The following release blockers are explicit, not confirmed exploitable vulnerabilities:

1. The runtime is a demonstration with schematic map/data and no station editor or provider integrations; it cannot be shipped as full HamClock parity.
2. The native UI has only mouse pane cycling and Escape, tiny bitmap text and no semantic accessibility tree. The accepted HTML study is separate from the executable.
3. The main loop redraws the entire scene continuously and adds a 25 ms delay, even while idle. Target CPU/latency evidence is required before choosing an update policy.
4. Exact origin-pole bearings currently return a numeric heading tied to the supplied longitude; documentation and tests must decide whether this convention is intended or bearings should be undefined.
5. Desktop build success does not demonstrate Pi 3/Pi 4 installation, performance or compatibility. Full parity, accessibility compliance, name clearance and distribution notices remain unverified.

## Before any release label

Require a commit-pinned parity ledger for the final original version, independent review of settings and network behavior, provenance for every redistributed asset/dependency, target hardware evidence, clean install/restart/recovery tests, and explicit user approval for the proposed release scope. If incomplete, label the deliverable a preview and list missing functions beside it; never present a deadline as evidence that a gate passed.
