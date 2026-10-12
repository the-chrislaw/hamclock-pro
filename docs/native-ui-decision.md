# Native UI framework decision

Research and local read-only inspection: 2026-10-03. No packages installed, source copied, assets downloaded or hardware benchmarks performed.

**Recommendation:** implement the next user-facing station dialog and accepted soft dashboard in **Qt 6 Widgets**, using only Core/Gui/Widgets and original C++/QPainter map drawing. Dynamically link approved LGPL modules. Keep station parsing, settings, geographic calculations and map coordinates independent of Qt so the working SDL experiment and tests remain useful. This is an engineering recommendation, not legal clearance or demonstrated Pi performance.

Confidence labels: **Verified** = primary documentation or local observation; **Inference** = proposed implementation; **Unknown** = must measure/check on the actual target.

## Why this choice

| Concern | Qt 6 Widgets | SDL2 + SDL_ttf |
|---|---|---|
| Station editor | Standard text fields, buttons, focus and layouts; apply existing original validation model. | Original controls need text editing, focus, caret, selection, keyboard and touch behavior implemented separately. |
| Accessibility | Documented widget semantic tree and platform bridges; custom map still needs descriptions/actions. | Text rendering alone supplies no semantic tree. A complete Linux bridge remains unverified and would be separate work. |
| Accepted appearance | System fonts/palette plus restrained original spacing, surfaces and focus styling. | Full drawing freedom and direct reuse of current renderer; every control state must be authored. |
| Compliance work | Module selection, LGPL notices/source/replacement conditions and transitive inventory. | SDL/SDL_ttf permissive licenses, plus separate font/FreeType/HarfBuzz notices; custom accessibility costs. |
| Pi cost | Unknown measured startup/RAM/redraw and compositor behavior. | Unknown measured cost too; smaller existing integration is not evidence of a complete accessible UI being faster. |

**Verified:** Qt documents built-in widget metadata, keyboard navigation, scalable layouts and system fonts/palette. It exposes custom accessibility interfaces rather than automatically making arbitrary painted content accessible. [Qt accessibility overview](https://doc.qt.io/qt-6/accessible.html), [QWidget accessibility](https://doc.qt.io/qt-6/accessible-qwidget.html). Its upstream Linux implementation contains AT-SPI/D-Bus adapters. That confirms a bridge exists, not that the user's Pi session has it running. [Qt Linux accessibility source inventory](https://github.com/qt/qtbase/tree/dev/src/gui/accessible/linux).

**Inference:** standard `QLineEdit`, labels with buddies, `QComboBox`, buttons and a dialog button box reduce custom control work. Expose DE/DX, distance, bearing and the route summary as regular readable widgets alongside the map. Supply a meaningful map accessible name/description; any selectable stations/keyboard map actions need their own accessible objects or equivalent standard controls. Test Orca on the actual desktop session and VoiceOver on the development Mac. Neither framework choice establishes WCAG/ADA compliance.

## License scope and distribution

**Verified:** Qt Core, Gui and Widgets documentation expressly offers LGPLv3 licensing; Gui also lists incorporated third-party components with their own terms. Choose and inventory actual installed versions, not merely the current website's version. [Core](https://doc.qt.io/qt-6/qtcore-index.html), [Gui](https://doc.qt.io/qt-6/qtgui-index.html), [Widgets](https://doc.qt.io/qt-6/qtwidgets-index.html).

**Verified:** LGPLv3 combined-work terms allow a suitable shared-library mechanism that operates with an interface-compatible modified library, require prominent notice and copies of LGPL/GPL texts, preserve library-modification/reverse-engineering rights, and require applicable installation information. Static linking has a different relinking/materials route; it is not automatically prohibited, but adds compliance work. [Actual LGPLv3 text hosted by Qt](https://doc.qt.io/qt-6/lgpl.html).

**Verified:** Qt's own obligations guidance additionally describes delivering corresponding library source or a written offer, retaining user replacement rights and providing notices. Dynamic linking alone does not satisfy every obligation. [Qt obligations guidance](https://www.qt.io/development/open-source-lgpl-obligations).

**Inference:** keep original application files MIT, retain each dependency's own license, and initially use system shared libraries. Before distributing binaries or a preinstalled image, produce a versioned notices/source provision record and test replacing an interface-compatible Qt library on an unlocked target. If bundling Qt, keep the exact corresponding source and build modifications under project control. Record separately whether dependencies are system-provided or actually redistributed; do not assume all packaging scenarios have identical duties.

**Verified:** Qt Virtual Keyboard is GPLv3/commercial, despite the base Widgets recommendation using LGPL modules. Do not add it under an assumption that all Qt modules have the same license. [Virtual Keyboard license page](https://doc.qt.io/qt-6/qtvirtualkeyboard-index.html). **Inference:** use the desktop's existing input method/on-screen keyboard or an original limited coordinate keypad made from standard buttons until keyboard integration and its licensing are reviewed.

**Verified:** SDL_ttf's SDL2 branch uses FreeType/HarfBuzz for text and has a zlib license; redistributable font rights are separate. [SDL_ttf SDL2](https://github.com/libsdl-org/SDL_ttf/tree/SDL2), [license](https://github.com/libsdl-org/SDL_ttf/blob/SDL2/LICENSE.txt). DejaVu's license permits redistribution subject to its notice and modification/naming conditions; retain the exact downloaded/distribution font notice if later bundled. [DejaVu terms](https://dejavu-fonts.github.io/License.html).

**Inference:** SDL text caches must have byte/count limits and eviction. One 1024 × 1024 RGBA atlas is 4 MiB before extra CPU/GPU copies; this arithmetic is not runtime measurement. SDL_ttf improves typography, but does not solve station-control semantics or screen-reader support. **Unknown:** a suitable maintained SDL Linux accessibility bridge and the cost of implementing one for all required controls.

## Local availability and minimal dependency plan

**Verified local:** Homebrew and `sdl2-config` exist; SDL2 compatibility package 2.32.72 and SDL3 3.4.16 are installed. No `qmake`, `qmake6`, `pkg-config` or CMake was found on PATH. `brew list --versions` found no Qt/qtbase, SDL_ttf, pkgconf or CMake package. The standard SDL_ttf header and Qt6 CMake directory were absent. A system Arial font exists on the Mac; its existence grants no redistribution rights. This inspection is not an exhaustive search of custom dependency directories.

**Verified package options:** Homebrew offers `qtbase` and `sdl2_ttf`; its full `qt` formula pulls many additional modules. Prefer the narrow base package. [qtbase](https://formulae.brew.sh/formula/qtbase), [full qt](https://formulae.brew.sh/formula/qt), [sdl2_ttf](https://formulae.brew.sh/formula/sdl2_ttf).

Proposed development installs, **not executed**:

- Mac Qt build: `brew install qtbase cmake`. Use the qtbase CMake prefix returned by Homebrew; no full Qt suite or Creator required.
- Debian-family Pi Qt build: `build-essential cmake qt6-base-dev`; add `qt6-wayland` when the actual desktop session requires its plugin. Use distribution dependency resolution instead of guessing individual runtime libraries. Debian Bookworm documents the base package at **6.4.2** and armhf/arm64 builds. [base package](https://packages.debian.org/bookworm/qt6-base-dev), [Wayland package](https://packages.debian.org/bookworm/qt6-wayland).
- Alternative SDL text spike: Mac `sdl2_ttf pkgconf`; Pi `libsdl2-dev libsdl2-ttf-dev pkg-config fonts-dejavu-core` plus compiler/build tools. [Debian SDL_ttf package](https://packages.debian.org/bookworm/libsdl2-ttf-dev).

**Unknown:** user's Raspberry Pi OS release/architecture, display resolution, session (X11/Wayland), current package versions, available on-screen keyboard and accessibility services. Resolve these before choosing the exact runtime package list. Do not infer Pi OS version from the board model.

## Two steps of implementation lookahead

### Step 1: usable native editor and theme

**Inference:** add a separate Qt executable/CMake target using the existing editor model, settings functions and map coordinates. A dialog edits a copy, validates through the same bounded parser and saves through existing persistence; Cancel makes no change and failures keep the dialog open with a precise visible/readable error. Keep original schema and storage protections intact rather than introducing a second settings format.

Draw map geometry in a custom QWidget through QPainter, cache static paths, and repaint changing overlays only. Use ordinary widgets for settings, toolbar and numeric information. Apply warm neutral opaque surfaces, modest corners, readable typography and visible focus; avoid blur/glass or animation needed to understand data. Restrict style sheets to bundled original constants: Qt documents local-file loading via stylesheet `url()` and cautions against arbitrary state blobs. [Widgets security considerations](https://doc.qt.io/qt-6/qtwidgets-index.html).

**Verified:** `QStyleHints::colorScheme` begins in Qt 6.5; its setter begins in 6.8. A Qt 6.4 baseline cannot use either unguarded. [QStyleHints API](https://doc.qt.io/qt-6/qstylehints.html). **Inference:** System mode follows application palette/style events on 6.4 with a documented fallback if the desktop reports no preference. Light/Dark explicitly apply application-owned palettes; later versions can use guarded color-scheme APIs. Recompute contrast for custom map colors after palette changes. Keep System, Light and Dark as distinct saved choices.

### Step 2: target deployment and acceptance

**Verified:** Linux deployment requires matching runtime libraries and platform plugins; applications cannot assume binaries built for another OS/ABI will work. Qt documents plugin paths and required platform integration. [Linux deployment](https://doc.qt.io/qt-6/linux-deployment.html), [Linux requirements](https://doc.qt.io/qt-6/linux-requirements.html).

**Inference:** build on or cross-compile for the selected Pi OS/architecture. Prefer a distribution package depending on system Qt to an opaque all-in-one image. Limit plugin paths to trusted installed locations; run as an ordinary user. Do not mix SDL and Qt event loops in the same window as the first port. Keep CLI domain checks independently runnable over SSH; browser control remains a separate authenticated service feature.

Acceptance before framework freeze: Pi 3 and Pi 4 with the actual seven-inch display; X11 and Wayland where supported; touch, keyboard Tab/Shift-Tab/Enter/Escape, selection/IME, focus restoration, on-screen keyboard occlusion, enlarged text/reflow, System changes and explicit themes. Verify accessible labels/roles/errors with Orca, plus cold startup/peak RSS/CPU and map redraw/interaction latency during a sustained run. Use identical workload and separate cold/cached redraw measurements for Qt and SDL. **Unknown:** all resulting measurements and any performance tradeoff. If Qt exceeds the target budget, first reduce full-window repaints, font/path cache churn and effects; do not discard accessibility without measuring the alternative.

The next concrete patch should be the Qt station editor and map-bearing dashboard shell—not another visual mockup. A successful local build and semantic-control test can establish that slice; Pi benchmarks, full HamClock 4.22 parity and final distribution compliance remain release gates.
