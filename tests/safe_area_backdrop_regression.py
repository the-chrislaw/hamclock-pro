#!/usr/bin/env python3
"""iPhone/iPad: the area outside the safe area takes the page colour, not black bands."""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent
main = (root / "src/ui/qml/Main.qml").read_text()
if not re.search(r'target:\s*safeArea;\s*property:\s*"backdrop";\s*value:\s*Theme\.canvas', main):
    sys.exit("Main.qml: bind safeArea.backdrop to Theme.canvas so the status bar area follows the theme")
native = (root / "src/platform/apple_safearea_ios.mm").read_text()
for needle in ("backgroundColor", "preferredStatusBarStyle", "UIStatusBarStyleDarkContent"):
    if needle not in native:
        sys.exit(f"apple_safearea_ios.mm: missing {needle} (backdrop fill and readable status bar text)")
print("ok")
