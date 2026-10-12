# Native typography and accessibility decision

Research date: 2026-10-02. This is a dependency proposal, not an installation or a compliance certification.

## Minimal next step

Keep the existing SDL2 experiment for one bounded visual-port milestone. Replace the pixel-letter routine with **SDL2_ttf and unmodified DejaVu Sans regular/bold**. Use blended UTF-8 rendering and cache rendered text textures. Measure actual text width and height rather than assuming a fixed character advance. Use mixed-case labels, regular body text, and bold only for hierarchy. Avoid synthetic bold and all-capital body copy.

This is a readily packaged, openly licensed baseline, not a claim that DejaVu reproduces Apple's typography. Apple's hierarchy, spacing, restrained surfaces and readable controls can inform our original design without distributing Apple fonts or artwork.

SDL2_ttf provides antialiased UTF-8 text through [TTF_RenderUTF8_Blended](https://wiki.libsdl.org/SDL2_ttf/TTF_RenderUTF8_Blended). It allocates a surface; free that surface after creating the texture and release cached textures on renderer destruction. Render at the actual output scale rather than enlarging a low-resolution glyph image. Re-render changing clock text at its update interval, not on every frame. Check font-open/render failures and show a useful diagnostic rather than silently falling back to rejected pixel lettering.

## Local availability

Inspected `/opt/homebrew/opt`, headers, package metadata and `/Library/Fonts` and the user font directory. SDL2/SDL2 compatibility and SDL3 are installed. No SDL2_ttf header/package metadata, DejaVu or Liberation font was found there. `/opt/homebrew/bin/pkg-config` is absent. System Apple fonts are present but are not approved distributable project assets. No dependencies were installed in this research task.

Suggested reputable package-manager dependencies:

| Environment | Packages | Notes |
| --- | --- | --- |
| Local Homebrew | `sdl2_ttf`, `pkgconf`; cask `font-dejavu` | Full font family contains regular and bold. The smaller `font-dejavu-sans` cask exposes only the regular face. Do not install both conflicting casks. |
| Raspberry Pi OS/Debian | `libsdl2-ttf-dev`, `fonts-dejavu-core`, `pkg-config`, existing SDL2/compiler packages | Verify the user's OS release repositories first. Debian lists ARM64 and ARMHF builds; this does not prove our executable's Pi compatibility. |

Homebrew currently reports SDL2_ttf 2.24.0 and depends on FreeType, HarfBuzz and SDL2-compat. This can change the local SDL backend; record resolved versions and re-run the existing native checks after installation. Do not assume local macOS renderer measurements predict the Pi's Linux renderer.

Sources: [Homebrew SDL2_ttf](https://formulae.brew.sh/formula/sdl2_ttf), [Homebrew complete DejaVu family](https://formulae.brew.sh/cask/font-dejavu), [Debian SDL2_ttf](https://packages.debian.org/trixie/libsdl2-ttf-dev), [Debian DejaVu core](https://packages.debian.org/trixie/fonts-dejavu-core).

## License and provenance gate

SDL2_ttf uses the [zlib license](https://github.com/libsdl-org/SDL_ttf/blob/SDL2/LICENSE.txt). Keep its source notice in source distributions and identify dependencies separately from our MIT application source. The package manager brings transitive libraries with their own notices; record them before bundling an executable.

DejaVu is **not MIT or simply public domain as a whole**. Its upstream [license page](https://dejavu-fonts.github.io/License.html) includes Bitstream Vera and Arev notices; DejaVu's changes are public domain. Redistribution requires the applicable font notices, and modified fonts have naming restrictions. Do not edit/subset/rename fonts in this milestone. Before bundling, retain the exact upstream/package license file, version, font-file names, checksums and package origin in the release notices. Using system-installed fonts avoids adding font binaries to this experiment, but does not remove license work for a future bundled release.

Prefer known distribution font paths (for example `/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf`), a documented user font-path override, and a packaged asset path only after the gate above. Do not search/download fonts at runtime. Treat external font files as input: cap supported file sizes, handle parse errors, and keep font libraries updated. No Apple SF/New York files are copied from the development machine.

## Native accessibility remains an architecture decision

Antialiased fonts, keyboard navigation and contrast checks do **not** provide a screen-reader interface. Our current SDL canvas has no implemented semantic accessibility tree. The browser prototype's HTML semantics do not automatically carry into this executable. Do not call either view ADA compliant.

Qt documents accessibility interfaces for widgets and Qt Quick Controls, with custom interfaces required for custom-rendered content: [Qt accessibility](https://doc.qt.io/qt-6/accessible.html) and [QWidget accessibility](https://doc.qt.io/qt-6/accessible-qwidget.html). This makes a semantic Qt shell worth evaluating before we expand the SDL application. Qt's module-specific license and deployment requirements need a separate review; do not adopt it solely on this font recommendation.

Bound the decision to two comparable offline shells on a real Pi 3: the accepted dashboard visual style with SDL2_ttf, and a Qt Widgets shell with standard controls plus an original custom map. Test Linux accessibility with Orca/AT-SPI under the chosen desktop session, including pane controls, clock data and a textual map alternative. If SDL would require a substantial custom accessibility bridge, prefer the standard-control shell unless measured hardware constraints rule it out. No full feature implementation should precede this decision.

Record OS/session/display, package versions, startup time, idle and updating CPU, resident memory, input response, text sharpness, and screen-reader results for each. Include 800×480 and larger display sizes; test 100/150/200% text scale without clipping or loss of actions. Contrast needs numerical testing in both themes, with labels/patterns for color-dependent data. A WCAG 2.2 AA-informed test plan is useful, but legal applicability and native assistive-technology behavior require their own assessment. Acceptance thresholds and touch target sizes should be chosen against the user's physical screen, viewing distance and input needs.

No target-hardware performance or assistive-technology measurements have been made in this task. The decision remains provisional until those measurements exist.
