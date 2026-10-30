#!/usr/bin/env python3
"""Map menu: chips show readable names (not raw ids) and the Flow cannot feed its width back into the popup."""
import pathlib, re, sys

qml = (pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml/MapMenu.qml").read_text()
if not re.search(r"label:\s*menu\.map\.layerName\(modelData\)", qml):
    sys.exit("MapMenu.qml: layer chips must use map.layerName(id)")
if not re.search(r"label:\s*menu\.map\.styleName\(modelData\)", qml):
    sys.exit("MapMenu.qml: base map chips must use map.styleName(id)")
flow = qml.split("Flow {", 1)[1].split("Repeater", 1)[0]
if "Layout.preferredWidth" not in flow:
    sys.exit("MapMenu.qml: the base map Flow needs a fixed preferred width (width binding loop)")
if not re.search(r"contentWidth:\s*width", qml) or not re.search(r"id: menuColumn\s*\n\s*width:\s*menuFlick\.width", qml):
    sys.exit("MapMenu.qml: the menu column must take the popup width, not its own childrenRect")
if "RowLayout" in qml:
    sys.exit("MapMenu.qml: chip rows must wrap (Flow), a RowLayout elides them on a phone")
main = (pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml/Main.qml").read_text()
menu_use = main.split("MapMenu {", 1)[1][:500]
if re.search(r"^\s*width:", menu_use, re.M) or "maxWidth:" not in menu_use:
    sys.exit("Main.qml: MapMenu takes a maxWidth and sizes itself on open (no width binding)")
if "onAboutToShow: fitWidth()" not in qml:
    sys.exit("MapMenu.qml: the width must be assigned on open (fitWidth)")

# Settings: the hidden (iOS) station hardware card has width 0; its wrapped labels then loop the
# layout polish. It lays out at its natural width while hidden.
settings = (pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml/SettingsPage.qml").read_text()
hardware = settings.split("id: hardwareCard", 1)[1][:200]
if not re.search(r"width:\s*parent\.width\s*>\s*0\s*\?\s*parent\.width\s*:\s*implicitWidth", hardware):
    sys.exit("SettingsPage.qml: hardwareCard must not lay out at zero width (polish loop)")
print("ok")
