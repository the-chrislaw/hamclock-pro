#!/usr/bin/env python3
"""Desktop keyboard shortcuts in Main.qml follow each platform's convention.

macOS reserves F11 (Show Desktop), so full screen also needs Control-Command-F,
and Command-comma ("Ctrl+," in QML, which Qt maps to Command on the Mac) opens
Settings as in every Mac app.
"""
import pathlib, re, sys

qml = (pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml/Main.qml").read_text()
shortcuts = re.findall(r"Shortcut\s*\{([^\n]*)\}", qml)
if not shortcuts:
    sys.exit("Main.qml: no Shortcut items found")


def find(pattern: str) -> list[str]:
    return [s for s in shortcuts if re.search(pattern, s)]


full = find(r"Window\.FullScreen")
if len(full) != 1:
    sys.exit("Main.qml: expected one full-screen Shortcut")
for seq in ('"F11"', '"Ctrl+Meta+F"'):
    if seq not in full[0]:
        sys.exit("Main.qml: full-screen Shortcut is missing " + seq)

prefs = find(r'"Ctrl\+,"')
if len(prefs) != 1 or not re.search(r"onActivated:\s*settingsLayer\.visible\s*=\s*true", prefs[0]):
    sys.exit("Main.qml: Ctrl+, (Command-comma) must open Settings")
if "enabled:" not in prefs[0]:
    sys.exit("Main.qml: Settings shortcut must be disabled while an overlay covers the page")

# Every sequence must be bound once only: Qt treats a sequence bound twice as
# ambiguous and fires neither shortcut.
seen = {}
for s in shortcuts:
    enabled = re.search(r"enabled:\s*([^;]+);", s)
    for seq in re.findall(r'"([^"]+)"|StandardKey\.(\w+)', s):
        key = seq[0] or seq[1]
        if key in seen and not (enabled and seen[key]):
            sys.exit("Main.qml: sequence bound twice without exclusive enabled: " + key)
        seen[key] = bool(enabled)
print("ok")
