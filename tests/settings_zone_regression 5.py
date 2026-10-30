#!/usr/bin/env python3
"""Settings: the station's time zone follows the station's location, never the device's.

Bug B2 (#702): with no saved zone, the picker showed timeZones.systemZone() and Done saved it,
so a Connecticut station set up on a Mac in California got Los Angeles time. Typing home
latitude/longitude by hand never suggested a zone either (only Find and the grid did).
"""
import pathlib, re, sys

qml = (pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml/SettingsPage.qml").read_text()

# 1. Every place that fills the station zone picker goes through timeZones.stationZone(saved, lat, lon).
zone_sets = re.findall(r"zone\.zoneId\s*=\s*Qt\.binding\([^\n]*|zoneId:\s*[^\n]*appSettings\.deZone[^\n]*", qml)
if len(zone_sets) < 2:
    sys.exit("SettingsPage.qml: expected the zone picker default and reload() binding")
for line in zone_sets:
    if "timeZones.stationZone(" not in line:
        sys.exit("SettingsPage.qml: station zone must come from timeZones.stationZone(): " + line.strip())
    if "systemZone" in line:
        sys.exit("SettingsPage.qml: station zone must not default to the device zone: " + line.strip())

# 2. Hand-typed home coordinates suggest a zone when editing finishes (like Find and the grid).
for field in ("deLat", "deLon"):
    block = re.search(r"target:\s*" + field + r"\.field.*?onEditingFinished\(\)\s*\{(.*?)\}", qml, re.S)
    if not block or "suggestFromTyped" not in block.group(1):
        sys.exit(f"SettingsPage.qml: {field} editingFinished must suggest the zone")

# 3. The manual "Suggest zone" button is gone: suggesting is automatic now.
if "Suggest zone from home coordinates" in qml:
    sys.exit("SettingsPage.qml: remove the 'Suggest zone from home coordinates' button")

# 4. "Set DE here" on the map moves the zone with the station (it kept the old zone).
tap = (pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml/MapTapMenu.qml").read_text()
set_de = re.search(r"function setDe\(\)\s*\{(.*?)\n    \}", tap, re.S)
if not set_de or "timeZones.suggestZone(latitude, longitude)" not in set_de.group(1):
    sys.exit("MapTapMenu.qml: setDe() must take the zone of the new DE location")
print("ok")
