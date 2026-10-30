#!/usr/bin/env python3
"""Offline check of the iOS 27 SDK launch requirements (docs/ios-uiscene.md).

Apps linked against the iOS 27 SDK must adopt the UIScene life cycle and must
declare a launch screen. Qt 6.8+ adopts scenes itself; older Qt kits rely on
src/platform/apple_scene_ios.mm. No Xcode or simulator needed.
"""
import pathlib
import plistlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
QT_SCENE_VERSION = (6, 8)
LAUNCH_SCREEN_KEYS = ("UILaunchStoryboardName", "UILaunchStoryboards", "UILaunchScreen", "UILaunchScreens")


def check_cmake_gate(errors: list[str]) -> None:
    cmake = (ROOT / "cmake/HamClockApp.cmake").read_text()
    gate = re.search(
        r"if\(IOS AND Qt6_VERSION VERSION_LESS 6\.8\)\s*\n\s*target_sources\(hamclock-pro PRIVATE src/platform/apple_scene_ios\.mm\)",
        cmake,
    )
    if not gate:
        errors.append("apple_scene_ios.mm must be added to hamclock-pro only for iOS with Qt < 6.8")
    if cmake.count("apple_scene_ios.mm") != 1:
        errors.append("apple_scene_ios.mm must be listed exactly once (behind the Qt < 6.8 gate)")


def check_shim_defers_to_qt(errors: list[str]) -> None:
    shim = (ROOT / "src/platform/apple_scene_ios.mm").read_text()
    if "instancesRespondToSelector:configure" not in shim:
        errors.append("shim must leave a Qt that already adopts scenes alone")
    if "QIOSApplicationDelegate" not in shim or "QUIWindow" not in shim:
        errors.append("shim must target Qt's QIOSApplicationDelegate and QUIWindow")


def check_plist(errors: list[str]) -> None:
    text = (ROOT / "packaging/apple/Info-ios.plist").read_text()
    # The ${...} line is a CMake substitution; drop it so the plist parses.
    text = re.sub(r"^\$\{[A-Z_]+\}\s*$", "", text, flags=re.M)
    plist = plistlib.loads(text.encode())
    if not any(key in plist for key in LAUNCH_SCREEN_KEYS):
        errors.append("Info-ios.plist needs a launch screen key (iOS 27 SDK requirement)")
    if "UIApplicationSceneManifest" in plist:
        errors.append("Info-ios.plist must not declare a scene manifest: Qt 6.8+ and the shim configure scenes in code")


def check_testflight_qt(errors: list[str]) -> None:
    workflow = (ROOT / ".github/workflows/apple.yml").read_text()
    match = re.search(r'QT_VER:\s*"?(\d+)\.(\d+)', workflow)
    if not match:
        errors.append("could not find QT_VER for the TestFlight job in apple.yml")
        return
    if (int(match.group(1)), int(match.group(2))) < QT_SCENE_VERSION:
        errors.append("TestFlight Qt %s.%s predates Qt's own UIScene support; see docs/ios-uiscene.md" % match.groups())


def main() -> int:
    errors: list[str] = []
    for check in (check_cmake_gate, check_shim_defers_to_qt, check_plist, check_testflight_qt):
        check(errors)
    for error in errors:
        print("FAIL:", error)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
