#!/usr/bin/env python3
"""App Store metadata checks for the iPhone/iPad, Mac and Apple TV bundles.

Catches the review and upload rejections that no compiler sees: a permission
API used without its usage string, a privacy manifest missing a required-reason
category, a missing launch screen or iPad orientation, and missing tvOS icon/top
shelf layers.
Runs on any host (plain plistlib, no Xcode).
"""
import json
import pathlib
import plistlib
import re
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
APPLE = ROOT / "packaging" / "apple"
TVOS = ROOT / "apple" / "tvos" / "App"
TV_BRAND = TVOS / "Resources" / "Assets.xcassets" / "App Icon & Top Shelf Image.brandassets"

ALL_IPAD_ORIENTATIONS = {
    "UIInterfaceOrientationPortrait", "UIInterfaceOrientationPortraitUpsideDown",
    "UIInterfaceOrientationLandscapeLeft", "UIInterfaceOrientationLandscapeRight",
}
MIN_USAGE_TEXT = 40  # App Review rejects vague one-word purpose strings (5.1.1)


def load_plist(path, substitutions=None):
    text = path.read_text(encoding="utf-8")
    for placeholder, value in (substitutions or {}).items():
        text = text.replace(placeholder, value)
    return plistlib.loads(text.encode("utf-8"))


def sources_matching(pattern, *dirs):
    regex = re.compile(pattern)
    hits = []
    for directory in dirs:
        for path in directory.rglob("*"):
            if path.suffix in {".mm", ".m", ".cpp", ".swift"} and regex.search(path.read_text(encoding="utf-8", errors="ignore")):
                hits.append(path)
    return hits


def api_reasons(manifest):
    return {entry["NSPrivacyAccessedAPIType"]: set(entry["NSPrivacyAccessedAPITypeReasons"])
            for entry in manifest["NSPrivacyAccessedAPITypes"]}


class IosBundle(unittest.TestCase):
    def setUp(self):
        sky_keys = (APPLE / "sky-finder-keys.plist.in").read_text(encoding="utf-8")
        self.plist = load_plist(APPLE / "Info-ios.plist", {"${HAMCLOCK_SKY_FINDER_PLIST_KEYS}": sky_keys})
        self.manifest = load_plist(APPLE / "PrivacyInfo.xcprivacy")

    def test_launch_screen_icon_and_ipad_multitasking(self):
        self.assertIn("UILaunchScreen", self.plist)
        self.assertEqual(self.plist["CFBundleIconName"], "AppIcon")
        self.assertEqual(set(self.plist["UISupportedInterfaceOrientations~ipad"]), ALL_IPAD_ORIENTATIONS)
        self.assertNotIn("UIRequiresFullScreen", self.plist)

    def test_export_compliance_answer_is_in_the_bundle(self):
        self.assertIs(self.plist["ITSAppUsesNonExemptEncryption"], False)

    def test_every_requested_permission_has_a_purpose_string(self):
        src = ROOT / "src"
        needed = {
            "NSLocationWhenInUseUsageDescription": r"CLLocationManager",
            "NSCameraUsageDescription": r"AVCaptureDevice",
            "NSMotionUsageDescription": r"CMMotionManager",
            "NSLocalNetworkUsageDescription": r"QUdpSocket|QTcpSocket|joinMulticastGroup",
        }
        for key, pattern in needed.items():
            if sources_matching(pattern, src):
                self.assertIn(key, self.plist, f"{key} missing but {pattern} is used")
                self.assertGreaterEqual(len(self.plist[key]), MIN_USAGE_TEXT, key)

    def test_privacy_manifest_declares_qt_and_app_required_reason_apis(self):
        self.assertIs(self.manifest["NSPrivacyTracking"], False)
        self.assertEqual(self.manifest["NSPrivacyTrackingDomains"], [])
        self.assertEqual(self.manifest["NSPrivacyCollectedDataTypes"], [])
        reasons = api_reasons(self.manifest)
        self.assertIn("CA92.1", reasons["NSPrivacyAccessedAPICategoryUserDefaults"])
        self.assertIn("C617.1", reasons["NSPrivacyAccessedAPICategoryFileTimestamp"])
        # Static Qt for iOS: QElapsedTimer uses mach_absolute_time.
        self.assertIn("35F9.1", reasons["NSPrivacyAccessedAPICategorySystemBootTime"])
        # The system pane shows free disk space to the user.
        self.assertIn("85F4.1", reasons["NSPrivacyAccessedAPICategoryDiskSpace"])


class MacBundle(unittest.TestCase):
    def test_local_network_prompt_text_and_encryption_answer(self):
        plist = load_plist(APPLE / "Info.plist.in")
        self.assertGreaterEqual(len(plist["NSLocalNetworkUsageDescription"]), MIN_USAGE_TEXT)
        self.assertIs(plist["ITSAppUsesNonExemptEncryption"], False)


class TvBundle(unittest.TestCase):
    def test_info_plist_and_privacy_manifest(self):
        plist = load_plist(TVOS / "Info.plist")
        self.assertIs(plist["ITSAppUsesNonExemptEncryption"], False)
        manifest = load_plist(TVOS / "Resources" / "PrivacyInfo.xcprivacy")
        self.assertIs(manifest["NSPrivacyTracking"], False)
        reasons = api_reasons(manifest)
        self.assertIn("CA92.1", reasons["NSPrivacyAccessedAPICategoryUserDefaults"])
        tv_sources = [TVOS / "Sources", ROOT / "apple" / "tvos" / "Core"]
        if sources_matching(r"systemUptime|mach_absolute_time", *tv_sources):
            self.assertIn("NSPrivacyAccessedAPICategorySystemBootTime", reasons)

    def test_layered_icons_and_top_shelf_images_are_present(self):
        brand = json.loads((TV_BRAND / "Contents.json").read_text(encoding="utf-8"))
        roles = {(a["role"], a["size"]) for a in brand["assets"]}
        self.assertEqual(roles, {("primary-app-icon", "1280x768"), ("primary-app-icon", "400x240"),
                                 ("top-shelf-image-wide", "2320x720"), ("top-shelf-image", "1920x720")})
        for asset in brand["assets"]:
            folder = TV_BRAND / asset["filename"]
            pngs = list(folder.rglob("*.png"))
            self.assertTrue(pngs, f"{asset['filename']} has no images")
            if folder.suffix == ".imagestack":
                layers = [p for p in folder.iterdir() if p.suffix == ".imagestacklayer"]
                self.assertGreaterEqual(len(layers), 2, f"{asset['filename']} needs at least two layers")


if __name__ == "__main__":
    sys.exit(0 if unittest.main(exit=False).result.wasSuccessful() else 1)
