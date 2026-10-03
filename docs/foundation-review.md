# Native foundation review

Review date: October 2, 2026 (America/Los_Angeles).

Status: first source and build-file review completed. No blocking security or licensing issue was identified in the narrow offline foundation inspected. This document is an engineering review, not legal clearance, a vulnerability certification, a distribution approval, or a feature-parity claim.

## Dependency evidence

The development machine exposes the SDL2 API through **sdl2-compat 2.32.72**, not the original SDL2 implementation. Its installed README says it uses SDL3 internally and requires SDL3 at runtime. Installed SDL3 is **3.4.16**. The installed license texts for both components use the zlib terms: origin must not be misrepresented, altered source must be marked, and the notice must remain in source distributions. These terms permit use by an MIT application; MIT does not replace either component's own notice.

Local evidence inspected:

- `/opt/homebrew/Cellar/sdl2-compat/2.32.72/LICENSE.txt`
- `/opt/homebrew/Cellar/sdl2-compat/2.32.72/README.md`
- `/opt/homebrew/Cellar/sdl3/3.4.16/LICENSE.txt`

This dependency is consistent with the existing [SDL publisher license](https://www.libsdl.org/license.php) finding in `outputs/hamclock-pro-legal-security-register.md`. No SDL_image, SDL_ttf, downloaded fonts, codecs, map assets, or HamClock source are approved by this review merely because SDL is permitted.

Before distributing packages, record the target OS, actual runtime versions, transitive libraries, redistribution notices, and package hashes. The Mac compatibility-layer result does not prove Raspberry Pi OS's SDL implementation, GPU/display stack, or dependencies behave identically. No vulnerability-database or complete transitive-license audit has been performed in this pass.

## Source review findings

Inspected `src/main.cpp`, `Makefile`, `README.md`, and `LICENSE`. The implementation agent declares the C++ rendering, bitmap glyphs, and schematic outlines original. No external asset files or copied archive implementation were present in the inspected source. A code inspection cannot independently establish authorship; keep the author's declaration and future provenance records. The UI prominently labels its data offline, illustrative and unimplemented; station location is unset. The original application has a local MIT notice.

Concrete inspection findings:

- Command-line input is restricted to no arguments, `--self-test`, or `--headless-frame PATH.bmp`; invalid forms return an error. Arguments are not interpolated into shell commands.
- Model state is fixed-size. Pane choices remain bounded by modulo arithmetic. Render geometry and drawing collections have fixed content; the rendered surface is 800 × 480. No untrusted feeds, imports, or arbitrary dimension input are accepted.
- SDL initialization and renderer creation failures are checked. Surfaces/windows are released on failed renderer setup; successful exits release renderer, surface, window and SDL state. Hardware acceleration has a software-rendering fallback.
- No application network requests/listeners, process execution, credential handling, telemetry, radio control, privilege elevation, or settings writes are present. This is a source-level statement about the application, not an audit of all operating-system/SDL internal behavior.
- The optional screenshot command writes **the user-specified path and can overwrite an existing file**. It checks the SDL save result. The Makefile preview chooses `build/dashboard.bmp`. Use a dedicated output path; this developer tool has no overwrite confirmation.
- Render calls and logical-size setup are not individually error-checked. This is a diagnostic/robustness improvement for future renderer validation, not an identified attacker-controlled path in this fixed offline demo. The live loop delays 25 ms per frame; real Pi resource measurements remain necessary.
- The Makefile invokes a local compiler and local `sdl2-config`; it does not fetch dependencies or execute downloaded install scripts. Build-tool path and environment variables are trusted local developer inputs. Its cleanup targets only its named binary and preview file.

The implementation agent reported successful compilation, self-tests, and a headless render. The integrator subsequently confirmed `make check preview` passes without compiler warnings. This reviewer did not independently repeat those runs. Self-tests cover pane geometry and selection, not full rendering, platform behavior, or security certification.

## Outstanding gates

### Station-calculation increment

The source now also accepts `--calculate DE_LAT DE_LON DX_LAT DX_LON`. Decimal arguments are bounded to 64 characters before allocation, parsed without shell execution, checked for finite values and geographic bounds. JSON output reports undefined bearings explicitly. Domain checks cover known distances, dateline crossings, poles, coincident/antipodal locations, locator precision, invalid inputs and longitude rounding at the eastern endpoint. A second review found the interior longitude rounding defect; it was corrected with an exclusive-endpoint clamp and a regression test before submission. No secrets, additional assets, data providers or runtime dependencies were introduced by the calculation work.

The earlier rendering/source observations above describe the initial experiment. The new domain/input tests extend that coverage; they do not validate the Pi hardware or production UI. The accepted design study is included separately under `design/`, with illustrative values, no live services, and accessibility testing still outstanding.

- Integrator-observed build/self-test/frame results and continued review as code changes.
- Actual Pi 3 execution, memory, CPU, frame responsiveness, display scaling, and failure behavior; Zero W remains exploratory.
- Complete HamClock 4.22 behavior inventory and feature acceptance tests.
- Provider rights, bounded feed parsing, TLS and stale-data handling before adding live inputs.
- Name/branding review before public launch.
- Dependency notices and provenance manifest before distributing a binary or source bundle.

Unknown archive file permissions block copying those files, not unrelated original implementation. A successful local demonstration does not establish full feature parity, legal clearance, security of future integrations, or hardware support.
