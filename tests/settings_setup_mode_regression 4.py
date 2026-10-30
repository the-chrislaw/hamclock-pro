#!/usr/bin/env python3
"""First run and DX in Settings (#702 items 5 and 8).

- With no station yet, Settings opens in setup mode: only the Station card, titled
  "Set up HamClock Pro", with Later and Start. The mode is decided when Settings opens, so
  the page does not change under the user when the station saves itself mid-typing.
- DX is not typed as coordinates in the Station card any more: it is shown read-only with
  "tap the map" (the map sets DX); typing a DX grid and the DX time zone are in Advanced.
"""
import pathlib, re, sys

page = (pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml/SettingsPage.qml").read_text()


def fail(msg):
    sys.exit("SettingsPage.qml: " + msg)


if not re.search(r"property bool setupMode:\s*false", page):
    fail("needs `property bool setupMode: false`")
reload = page.split("function reload()", 1)[1].split("\n    }\n", 1)[0]
if not re.search(r"setupMode\s*=\s*!appSettings\.configured", reload):
    fail("reload() must latch setupMode = !appSettings.configured when Settings opens")
if re.search(r"setupMode:\s*!appSettings\.configured", page):
    fail("setupMode must not be bound to configured (the page would change while typing)")

title_bar = page.split("id: titleBar", 1)[1].split("Flickable {", 1)[0]
for text in ('"Set up HamClock Pro"', '"Later"', '"Start"'):
    if text not in title_bar:
        fail(f"setup mode title bar needs {text}")
if "closeRequested()" not in title_bar.split('"Later"', 1)[1].split("}", 1)[0]:
    fail("Later closes without asking for the missing station")

more = re.search(r"id:\s*moreSections.*?visible:\s*!page\.setupMode", page, re.S)
if not more:
    fail("every section after Station sits in moreSections, hidden in setup mode")
station_at = page.index('title: "Station"')
more_at = page.index("id: moreSections")
for marker in ("AccountsSection", "DisplaySection", "ScreenSection", 'id: spotsCard', 'id: advancedSection', 'id: aboutSection'):
    if page.index(marker) < more_at:
        fail(f"{marker} must be inside moreSections")

station = page[station_at:more_at]
if re.search(r"id:\s*dx(Lat|Lon)\b", station):
    fail("DX latitude/longitude fields are gone from the Station card")
if "tap the map" not in station.lower():
    fail("the Station card shows DX read-only with 'tap the map'")
advanced = page.split("id: advancedSection", 1)[1].split("id: aboutSection", 1)[0]
if "id: dxGrid" not in advanced or "id: dxZone" not in advanced:
    fail("Advanced holds the DX grid field and the DX time zone")
print("ok")
