#!/usr/bin/env python3
"""Settings save as you go: no Save buttons, Done and Escape only close (owner decision, #702).

Fixes B3 (banner title lost on Done) and B4 (switches in one card saved three different ways).
Source checks, like the other QML regressions in tests/.
"""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml"
page = (root / "SettingsPage.qml").read_text()
sheet = (root / "AccountSheet.qml").read_text()
main = (root / "Main.qml").read_text()
display = (root / "DisplaySection.qml").read_text()


def fail(msg):
    sys.exit(msg)


# 1. No Save buttons left in Settings (a sheet's "Sign in"/"Connect" checks a login; that's an action).
for name, text in (("SettingsPage.qml", page), ("AccountSheet.qml", sheet)):
    for m in re.finditer(r'text:\s*"(Save(?: key)?)"', text):
        fail(f"{name}: Save button '{m.group(1)}' left; changes apply as they are made")
    if re.search(r'"Save key"', text):
        fail(f"{name}: 'Save key' button left")

# 2. Text fields save when editing finishes: each card wires its fields to its save().
for card in ("clusterCard", "hardwareCard", "stationCard"):
    block = page.split("id: " + card, 1)
    if len(block) < 2:
        fail("SettingsPage.qml: missing " + card)
    if "editingFinished.connect" not in block[1][:12000]:
        fail(f"SettingsPage.qml: {card} fields must save on editingFinished")

# 3. B3: the banner title saves when editing finishes (it used to need its own Save; Done dropped it).
if not re.search(r"id:\s*bannerTitle[^\n]*\n?.*?onEditingFinished:\s*appSettings\.saveBannerTitle", display, re.S):
    fail("DisplaySection.qml: banner title must save on editingFinished")

# 4. B4: every switch in the spots card applies at once, with a label that does not flip.
for sid, call in (("udpLan", "saveUdpSpots"), ("udpMe", "saveUdpScope"), ("udpSetsDx", "saveUdpSetsDx"),
                  ("pskWspr", "savePskWspr"), ("pskRbn", "savePskRbn")):
    m = re.search(r"ToggleChip\s*\{\s*id:\s*" + sid + r"\b(.*?)\n\s*(?:[A-Z]\w*\s*\{|\})", page, re.S)
    if not m:
        fail(f"SettingsPage.qml: {sid} must be a ToggleChip (fixed label, state shown by the chip)")
    if "appSettings." + call not in m.group(1):
        fail(f"SettingsPage.qml: {sid} must save at once with appSettings.{call}")
    if re.search(r"label:\s*on\s*\?|label:\s*\w+\s*\?", m.group(1)):
        fail(f"SettingsPage.qml: {sid} label must not change with its state")

chip = (root / "ToggleChip.qml").read_text()
if "property bool on" not in chip or "property string label" not in chip:
    fail("ToggleChip.qml: needs `on` and a fixed `label`")

# 5. Done and Escape keep what was typed (they flush the field being edited, then close).
if not re.search(r'text:\s*"Done".*saveAndClose\(\)', page):
    fail("SettingsPage.qml: Done must call saveAndClose()")
esc = [m.group(0) for m in re.finditer(r'Shortcut\s*\{[^}]*"Escape"[^}]*\}', main) if "enabled: settingsLayer.visible" in m.group(0)]
if len(esc) != 1 or "saveAndClose()" not in esc[0]:
    fail("Main.qml: Escape in Settings must keep changes (saveAndClose), not discard them")
print("ok")
