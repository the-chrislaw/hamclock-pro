#!/usr/bin/env python3
"""Settings layout (#702 items 6 and 9b): one Display card and one Screen card replace eleven
small cards, and the Data sources attribution sits at the bottom of the page."""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml"
page = (root / "SettingsPage.qml").read_text()
display = (root / "DisplaySection.qml").read_text()
screen = (root / "ScreenSection.qml").read_text()

gone = ["Appearance", "Distance units", "Clock format", "Callsign banner", "Text size", "Screen rotation",
        "Map size", "Accent colour", "Big Clock face", "TV overscan", "Screen blanking", "Exit"]
for title in gone:
    if re.search(r'title:\s*"' + re.escape(title) + '"', page):
        sys.exit(f"SettingsPage.qml: the '{title}' card belongs in the Display or Screen section now")

for section in ("DisplaySection", "ScreenSection"):
    if not re.search(section + r"\s*\{", page):
        sys.exit("SettingsPage.qml: missing " + section)

# Everything the old cards set is still settable, at once.
for call in ("appSettings.appearance", "saveImperialUnits", "saveClockFormat", "saveWeekStart", "saveTextScale",
             "saveAccentStyle", "saveMapSize", "saveBigClockFace", "saveOrientationLock", "saveBannerStyle",
             "saveBannerBackground", "saveBannerTitle"):
    if call not in display:
        sys.exit("DisplaySection.qml: missing " + call)
for call in ("saveBlanking", "saveBlankDays", "saveOverscan", "Qt.quit()"):
    if call not in screen:
        sys.exit("ScreenSection.qml: missing " + call)
if 'Qt.platform.os !== "ios"' not in screen.split("ColumnLayout", 1)[0]:
    sys.exit("ScreenSection.qml: the Screen card is not for iPhone/iPad")
if "!Theme.phone" not in display or 'Qt.platform.os === "ios"' not in display:
    sys.exit("DisplaySection.qml: Map size is hidden on phones and Screen rotation is iOS-only")

# Data sources is the last card in the page.
cards = [m.start() for m in re.finditer(r"^ {12}(?: {4})?[A-Z]\w*\s*\{", page, re.M)]   # top level, or inside moreSections
last = page[cards[-1]:]
if not re.search(r'title:\s*"(About and data sources|Data sources)"', last[:300]) or "dataSources" not in last:
    sys.exit("SettingsPage.qml: Data sources must be the last section")
print("ok")
