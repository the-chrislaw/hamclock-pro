#!/usr/bin/env python3
"""Keyboard / TV remote: a focused pane steps with Left/Right and opens options with Menu."""
import pathlib, re, sys

qml = (pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml/PaneSlot.qml").read_text()
want = {
    "Keys.onLeftPressed": r"step\(-1\)",
    "Keys.onRightPressed": r"step\(1\)",
    "Keys.onUpPressed": r"moveFocus\(false\)",
    "Keys.onDownPressed": r"moveFocus\(true\)",
    "Keys.onPressed": r"blanking\.touch\(\)",
    "Keys.onMenuPressed": r"optionsRequested",
}
for key, call in want.items():
    m = re.search(re.escape(key) + r".*", qml)
    if not m or not re.search(call, m.group(0)):
        sys.exit("PaneSlot.qml: missing " + key)
if "pageDots" not in qml or "Theme.phone" not in qml.split("pageDots", 1)[1][:200]:
    sys.exit("PaneSlot.qml: missing phone page dots")
print("ok")
