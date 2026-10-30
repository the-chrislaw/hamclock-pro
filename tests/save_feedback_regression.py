#!/usr/bin/env python3
"""Settings Save buttons confirm a successful save on the button itself ("Saved")."""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml"
button = (root / "AppButton.qml").read_text()
if not re.search(r"function confirm\(", button) or "Timer" not in button:
    sys.exit("AppButton.qml: needs confirm() that shows a short-lived 'Saved' label")

settings = (root / "SettingsPage.qml").read_text()
saves = [m.start() for m in re.finditer(r'AppButton \{ text: "Save( key)?"', settings)]
if len(saves) < 7:
    sys.exit(f"SettingsPage.qml: expected at least 7 Save buttons, found {len(saves)}")
for start in saves:
    handler = settings[start:start + 400].split("onClicked:", 1)[1].split("\n", 2)
    body = " ".join(handler[:2])
    if "confirm()" not in body:
        line = settings.count("\n", 0, start) + 1
        sys.exit(f"SettingsPage.qml:{line}: Save button does not confirm() on success")
print("ok")
