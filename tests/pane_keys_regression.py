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
if "PageDots {" not in qml or "Theme.phone" not in qml.split("PageDots {", 1)[1][:200]:
    sys.exit("PaneSlot.qml: missing phone page dots")
# Swipe carousel (pager.mjs holds the maths, tests/pager_regression.cpp checks it):
# the card follows the finger, release decides by distance and velocity, and keys
# and rotation slide through the same animation instead of swapping the card.
swipe = {
    "pager module": r'import "pager\.mjs" as Pager',
    "finger tracking": r"onCentroidChanged:.*dragTo\(",
    "release decision": r"Pager\.releaseStep\(",
    "step slides": r"function step\(direction\)[^}]*slideTo\(",
    "rotation slides": r"onTriggered:.*slideTo\(slot\.rotationNext",
    "neighbours preloaded": r"preload && \(id === prevId \|\| id === nextId",
    "slide lands on finish": r"onFinished: slot\.finishSlide\(\)",
    "dots follow the swipe": r"progress: .*slot\.offset",
}
for what, pattern in swipe.items():
    if not re.search(pattern, qml):
        sys.exit("PaneSlot.qml: swipe carousel lost " + what)
if re.search(r"active: widgetId === slot\.choice", qml):
    sys.exit("PaneSlot.qml: only the shown card is loaded; neighbours would slide in blank")
root = pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml"
big = (root / "BigClockPage.qml").read_text()
main = (root / "Main.qml").read_text()
timers = (root / "TimersPage.qml").read_text()
if "closeRequested()" not in big or "TapHandler" not in big:
    sys.exit("BigClockPage.qml: tap must close")
# 12-hour mode makes the seconds field "18 PM"; Number() turns that into NaN and the hands vanish.
if re.search(r"Number\(parts\[", big):
    sys.exit("BigClockPage.qml: analog hands must parse 12-hour time (use parseInt)")
if "bigClockRequested" not in timers or "bigClockLayer" not in main or "BigClockPage" not in main:
    sys.exit("Big Clock is not wired from the Timers page")
settings = (root / "SettingsPage.qml").read_text()
if "Qt.quit()" not in settings or "exitCard.armed" not in settings:
    sys.exit("SettingsPage.qml: Exit button must need a second tap")
# Every registered widget needs a category in the pane chooser, or it lands under "Other".
chooser = (root / "PaneChooser.qml").read_text()
ids = set()
for f in (root.parent.parent).rglob("*.cpp"):
    ids.update(re.findall(r'registry\.add\(\{"([a-z0-9_]+)"', f.read_text(errors="ignore")))
if not ids:
    sys.exit("no registered widgets found")
missing = sorted(i for i in ids if not re.search(r"\b" + i + r":", chooser))
if missing:
    sys.exit("PaneChooser.qml: no category for " + ", ".join(missing))
print("ok")
