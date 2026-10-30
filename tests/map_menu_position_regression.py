#!/usr/bin/env python3
"""The Map view menu opens below the "Map" button, never over it.

The button sits in the map card's body. With the card title shown (iPad, desktop)
the body starts below the title row, so a menu parented to the card but placed at
the button's body-relative y lands on top of the button. The menu must share the
button's parent (the map body) and be limited by the map's height.
"""
import pathlib
import sys

QML = pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml/Main.qml"


def block(text: str, start: str) -> str:
    """Return the brace-balanced QML block that opens at `start`."""
    i = text.find(start)
    if i < 0:
        sys.exit("Main.qml: no " + start)
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return text[i:j + 1]
    sys.exit("Main.qml: unbalanced " + start)


menu = block(QML.read_text(), "MapMenu {")
if "y: mapMenuButton.y + mapMenuButton.height" not in menu:
    sys.exit("Main.qml: MapMenu must open just below the Map button")
if "parent: mapCard" in menu:
    sys.exit("Main.qml: MapMenu is parented to the card but placed in body coordinates, "
             "so it covers the Map button when the card title shows")
if "worldMap.height" not in menu or "worldMap.width" not in menu:
    sys.exit("Main.qml: MapMenu size must be limited by the map it opens over")
# Tapping the Map button while the menu is open must close it: the press outside
# closes the popup, so the click that follows must not open it again at once.
if "onClosed: mapMenuButton.closedAt = Date.now()" not in menu:
    sys.exit("Main.qml: MapMenu must record when it closed")
if "Date.now() - mapMenuButton.closedAt" not in QML.read_text():
    sys.exit("Main.qml: the Map button must not re-open the menu it just closed")
print("ok")
