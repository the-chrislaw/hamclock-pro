#!/usr/bin/env python3
"""Settings progressive disclosure (#702 items 4, 9a and bug B6).

Expert settings start collapsed behind one row with a one-line summary; the spots card is split
into collapsed sub-groups; map label style and label gap are explicit choices, not chips that
silently cycle on every tap."""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml"
page = (root / "SettingsPage.qml").read_text()
group = (root / "SettingsGroup.qml").read_text()


def fail(msg):
    sys.exit(msg)


if not re.search(r"property bool expanded:\s*false", group):
    fail("SettingsGroup.qml: groups start collapsed")
if "summary" not in group or "elideOk" not in group:
    fail("SettingsGroup.qml: needs a one-line summary that may be cut short")

groups = {}
for m in re.finditer(r"SettingsGroup\s*\{", page):
    t = re.search(r'title:\s*"([^"]+)"', page[m.start():m.start() + 300])
    if t:
        groups[t.group(1)] = page[m.start():m.start() + 600]

top = ["Station hardware", "Advanced", "About and data sources"]
sub = ["Watch list", "PSKReporter", "Parks and summits on the air", "APRS", "Meshtastic", "WSJT-X / JTDX (UDP)",
       "ADIF log", "Spots on the map"]
for title in top + sub:
    if title not in groups:
        fail(f"SettingsPage.qml: missing collapsed group '{title}'")
    if not re.search(r"summary:\s*\S", groups[title]):
        fail(f"SettingsPage.qml: group '{title}' needs a summary")
    if re.search(r"expanded:\s*true", groups[title]):
        fail(f"SettingsPage.qml: group '{title}' must start collapsed")
for title in sub:
    if "boxed: false" not in groups[title]:
        fail(f"SettingsPage.qml: '{title}' is a row inside the Spots and feeds card (boxed: false)")

adv = page.split('title: "Advanced"', 1)[1].split("SettingsGroup {", 1)[0]
if "saveConfig" not in adv or "demoMode" not in adv:
    fail("SettingsPage.qml: Advanced holds Saved configurations and Demo mode")
about = page.split('title: "About and data sources"', 1)[1]
if "dataSources" not in about:
    fail("SettingsPage.qml: About and data sources lists the data sources")
if re.search(r'^            Card \{\s*\n\s*title:\s*"(Saved configurations|Demo mode|Data sources)"', page, re.M):
    fail("SettingsPage.qml: Saved configurations, Demo mode and Data sources are no longer top-level cards")

# B6: no tap-to-cycle chips for map labels or label gap.
if re.search(r"mode\s*>=\s*3\s*\?\s*-1\s*:\s*mode\s*\+\s*1", page) or re.search(r"px\s*>=\s*100\s*\?\s*0\s*:\s*px\s*\+\s*20", page):
    fail("SettingsPage.qml: map label chips still cycle on tap (B6); use explicit choices")
if not re.search(r'ChoiceRow\s*\{[^}]*label:\s*modelData\[1\]\s*\+\s*" labels"', page, re.S):
    fail("SettingsPage.qml: each source's map label style is a ChoiceRow")
if not re.search(r'ChoiceRow\s*\{[^}]*label:\s*"Space between map labels"', page, re.S):
    fail("SettingsPage.qml: label gap is a ChoiceRow")
print("ok")
