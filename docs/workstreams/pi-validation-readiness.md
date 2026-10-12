# Raspberry Pi validation readiness

Research checkpoint: 2026-10-11. Research only: no Pi contacted, packages installed, OS changed, device benchmarks run or new code introduced. Independent verification is required before promoting this plan. Confidence: **high** for cited platform/package facts and inspected repository requirements; **medium** for the proposed deployment path; **unverified** for application compatibility, performance, touch and accessibility on any Pi.

## What is established

The user's first target is Pi 4 with a seven-inch touch display; its RAM, screen model/resolution and OS are unknown. Pi 3 remains a required validation target. Raspberry Pi's [OS documentation, Architecture and Editions](https://www.raspberrypi.com/documentation/computers/os.html#architecture) identifies Pi 3/4 as 64-bit-capable, provides 32/64-bit OS editions, and distinguishes graphical Desktop from command-line Lite. It currently describes Trixie as current and Bookworm as previous. This is platform eligibility, not evidence that our program works. Its memory advantage statement for 32-bit is general guidance; our application needs measurement. [Pi 3 Model B specifications](https://www.raspberrypi.com/products/raspberry-pi-3-model-b/#specification) establish a 64-bit CPU and 1 GB RAM; obtain the actual B/B+ revision rather than generalizing all Pi 3 variants.

[Debian Bookworm qt6-base-dev](https://packages.debian.org/bookworm/qt6-base-dev) lists 6.4.2+dfsg-10 and arm64/armhf packages. [Debian Trixie qt6-base-dev](https://packages.debian.org/trixie/qt6-base-dev) lists 6.8.2+dfsg-9+deb13u2 and both architectures. These versions exceed our declared minimum. They establish Debian package availability, **not** the user's installed Raspberry Pi OS package candidate, repository configuration or working graphical plugin. Pin the exact on-device packages before testing. The 32-bit armhf userland is a separate ABI from arm64; a 64-bit kernel alone does not identify it. Do not add compiler architecture tuning until the intended image and CPU ABI are verified.

## Actual repository requirements and API gates

Inspected `CMakeLists.txt`, `Makefile`, `docs/native-ui-decision.md`, native entry points/appearance/map code and `src/providers/noaa_kp_parser.cpp`. CMake requires CMake >=3.16, C++17 and Qt >=6.4 Core/Gui/Widgets. Python 3 is required when `BUILD_TESTING` is enabled. Native targets are editor, map preview and dashboard; the offline Kp decoder links Qt Core and performs no live transport. The Makefile's SDL build and `make verify` are a different path and require SDL2 development discovery through `sdl2-config`; they do not substitute for CMake native/provider regressions. The earlier decision's October 3 tool-install snapshot is historical, not the current Mac inventory.

| Current usage | Primary documentation / version issue | Required evidence |
|---|---|---|
| QWidget/QDialog/QLineEdit/QComboBox, QPainter/QPainterPath winding fill, QImage, QPalette/events | Base Widgets/Gui design; current [Widgets](https://doc.qt.io/qt-6/qtwidgets-index.html) documents the module. | Compile all native targets against actual 6.4.2 and selected Pi Qt; render/unit tests then actual desktop interaction. Module presence does not certify every overload. |
| QStringDecoder, Stateless flag, QByteArrayView, qsizetype; QJsonDocument::fromJson, QLocale conversion | [QStringConverter](https://doc.qt.io/qt-6/qstringconverter.html#Flag-enum), [QStringDecoder](https://doc.qt.io/qt-6/qstringdecoder.html), [QJsonDocument](https://doc.qt.io/qt-6/qjsondocument.html). Current docs describe decoder errors and these APIs; archived 6.4/source-tag retrieval failed in this research. | Build/rerun parser regressions on baseline Qt and each userland. Do not treat current docs or host Qt as a complete baseline API audit. |
| QStyleHints::setTabFocusBehavior in all three native entry points | [QStyleHints current docs](https://doc.qt.io/qt-6/qstylehints.html#tabFocusBehavior-prop) expose the property; the setter is not clearly enumerated there. Its exact baseline declaration remains a compile gate here. | Qt 6.4.2 build verifies the actual headers. Preserve keyboard focus behavior if a compatibility fix is needed; do not silently remove it. |
| System theme uses application palette/events and contextual QTimer::singleShot | [QStyleHints](https://doc.qt.io/qt-6/qstylehints.html#colorScheme-prop) labels colorScheme since 6.5 and its setter since 6.8. Current code uses neither. | Palette propagation on the actual desktop; test System switching separately from explicit Light/Dark. Do not introduce newer APIs unguarded. |

No baseline build was run in this research. The Qt URLs currently resolve to newer documentation; they are not proof of Qt's supported platform policy for every older release. [Linux deployment](https://doc.qt.io/qt-6/linux-deployment.html#shared-libraries) establishes shared-library/plugin placement as a runtime concern; its newer deployment API example requires 6.5+, so it cannot be adopted unguarded for our 6.4 baseline.

## Proposed evidence matrix

Each row records exact source commit, board revision/RAM, OS image/release, kernel and **userland** architecture, compiler/CMake/Qt package versions, graphical session and plugin, screen pixels/logical scale, power/thermal conditions and test logs. All rows are currently **not run**. No row may be inferred from another.

| Row | Purpose | Required result before claim |
|---|---|---|
| Pi 4, user's actual Desktop OS/architecture | First acceptance target | Native build + CTest + SDL/CLI checks when dependencies approved; physical seven-inch interaction and resource run. |
| Pi 3, arm64 Desktop, pinned selected release | 64-bit lower-memory hardware path | Same tests, peak RSS/startup/paint/idle and sustained measurements; explicit board/RAM identity. |
| Pi 3, armhf Desktop, pinned selected release | 32-bit alternative and ABI coverage | Rebuild from source, integer/time/JSON/geometry regressions and physical interaction. No reused arm64 executable. |
| Linux arm64 or armhf with exact Qt 6.4.2 (Bookworm candidate) | Declared minimum API gate | Clean full CMake build and CTest; still not device or physical-display acceptance. |
| Selected newer Pi Qt (Trixie candidate 6.8.2) | Forward version/session gate | Clean build/regressions and actual selected plugin; does not establish minimum-version support. |

Proposed authorized execution protocol after device access/dependencies are arranged:

```sh
cmake -S . -B build/pi-native -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/pi-native --parallel 1
ctest --test-dir build/pi-native --output-on-failure
make verify
```

Use a fresh architecture-specific build directory. Parallel 1 is a conservative starting policy, not a benchmark; increase only after measuring available memory. `make verify` needs its separate SDL prerequisites; record a missing dependency as blocked, never as pass. Run a separate sanitizer build with supported target toolchain flags and strict warnings; sanitizer results do not replace release performance measurements. Record the test list from CTest at that commit, rather than hard-coding a count while development adds cases. Never run an untrusted binary through dependency inspection; inspect only our freshly built artifacts. Confirm dynamic Qt linkage and resolved trusted runtime/plugin paths.

Proposed dependency inventory: C++17 compiler/build tools, CMake, Python3 and qt6-base-dev with its development tools/QPA dependency packages. Actual Wayland sessions may need separately packaged Qt Wayland integration; X11 requires the corresponding XCB runtime dependencies. Resolve from the selected OS package metadata before requesting installation. SDL2 development is for the Make path. No QtNetwork, QtQuick or QtVirtualKeyboard is currently required. Lite lacks a desktop; adding a compositor/input stack is a separate decision, not a smaller drop-in equivalent.

## Physical-device checks and measurements

These are original acceptance proposals, not upstream guarantees or compliance certification:

1. Start the freshly built dashboard/editor/map preview in the user's local graphical session as an ordinary user. Verify startup, native fonts/fallback glyphs, DE/DX editing/apply/cancel, error readability and saved settings after restart. Use a separate disposable test settings directory where supported; never overwrite the user's HamClock settings.
2. Test actual seven-inch resolution/scale, touch target reachability, repeated taps, focus, text selection, keyboard Tab/Shift-Tab/Enter/Escape, IME and the existing desktop on-screen keyboard. Check keyboard occlusion and scroll/reflow at enlarged text. Host 800x480/1280x800 raster fixtures are coverage, not screen-driver evidence.
3. Exercise System/Light/Dark, desktop theme changes, resize/letterboxing, repaint after hide/show, map land/ocean/Caspian hole/Antarctica, routes/markers over fill. Inspect clipping, font contrast/focus and renderer regressions. Independent computer-control observations on the built Pi app are required; offscreen tests cannot detect input/compositor issues.
4. Run the desktop's existing accessibility bridge/screen reader, if available, to inspect labels, roles, errors, focus and custom-map equivalent descriptions. Installing Orca or another tool requires separate authorization. A working bridge does not establish ADA/WCAG compliance.
5. Measure cold/warm startup, peak/resident memory, idle and active CPU, cached/uncached map repaint and interaction latency under a fixed workload. Record distribution/percentiles and sustained behavior (proposed 30-minute run), temperature, throttling and power conditions. Raspberry Pi's [OS diagnostics](https://www.raspberrypi.com/documentation/computers/os.html#get_throttled) define throttling flags; use OS process memory evidence rather than interpreting source asset byte count as RAM. No product resource/latency budget has yet been demonstrated.
6. Test desktop sleep/display off and recovery, interrupted process/restart/settings recovery, and injected clock discontinuity/overflow regressions. Do not change the device clock or suspend it during unrelated work without explicit permission. The pure stopwatch model's injected monotonic time is not evidence about the eventual Linux clock adapter's behavior across suspend; define and test that adapter separately.

Browser control/local server authentication, SSH-only administration and a headless display backend remain distinct feature/deployment gates. Current native targets do not demonstrate those capabilities.

## Redistribution engineering gates

[Qt LGPL obligations guidance](https://www.qt.io/development/open-source-lgpl-obligations) requires more than choosing dynamic linking: users retain library replacement/modification rights, notices/license texts and the applicable library source provision/offer must be arranged, including relevant installation information. Some Qt modules are GPL-only. [LGPLv3 text, sections 4–5](https://doc.qt.io/qt-6/lgpl.html) describes combined-work/shared-library and installation conditions. This is an engineering checklist requiring review of the actual distribution; it is not legal certification.

Before a binary/image release: inventory exact Qt modules, shared libraries/plugins, distribution patches and other shipped dependencies/fonts; retain their licenses/notices, record whether each is system-provided or actually redistributed, and preserve the application's original MIT provenance separately. For redistributed Qt, archive/provide the exact corresponding source and any modifications/build material through a reviewed compliant route; do not assume an upstream URL to a different version suffices. Supply prominent notices and LGPL/GPL texts where applicable, and ensure packaging/EULA/device restrictions permit the required replacement/debugging rights. Demonstrate an interface-compatible replacement can be used on the intended unlocked target. A system-package dependency and a bundle/preinstalled image need separate review of what we distribute. Static Qt packaging is a separate relinking/materials analysis, not an automatic prohibition; it is outside this preferred first path.

## Two paths, two steps ahead

**A — Native build with OS shared Qt (preferred first experiment).** Step 1: read-only device/version inventory, user-approved dependency install, clean on-Pi build/regressions and physical QA. Step 2: measure bottlenecks and package only for demonstrated OS/ABI combinations with explicit system dependencies and notices. If Pi 3 memory/paint fails the eventual budget, first measure caches/repaint scope and build parallelism; do not assume changing framework fixes it. Confidence: medium for practical simplicity, unverified for this application.

**B — Cross-build against an exact target sysroot.** Step 1: establish target image/toolchain/Qt/sysroot identity and reproducible ARM artifact build; cross compilation must not accidentally execute target tests on the Mac. Step 2: transfer through a user-authorized path, run all target regressions and physical QA, then package for that pinned ABI. Qt plugins, C++ runtime and libc must match the intended target. This reduces build load on the Pi but adds toolchain/provenance drift risks; a cross-build success alone gives no performance/input evidence. No new toolchain is authorized by this document.

Minimal eventual user information: exact Pi 3/4 model and RAM, OS release and 32/64-bit userland, desktop versus Lite and X11/Wayland session, screen make/model/resolution/connection, existing keyboard/input method, available disk/memory, and preferred authorized access/testing method. Never store credentials, passwords, SSH keys or private host identifiers in this research record. Current work can continue without those details; they become required when device validation starts.

Suggested exclusive tickets: PR0 device evidence owner (read-only inventory); PR1 baseline Linux/Qt6.4 build owner (isolated build/config only); PR2 independent Pi regression verifier; PR3 physical QA owner (observations, no simultaneous display manipulation); PR4 resource measurement owner (fixed workload, no competing benchmark process); PR5 packaging/provenance owner. Root coordinates shared build/source fixes, and another agent verifies every support claim against logs. Freeze this research separately from any later implementation or hardware evidence.
