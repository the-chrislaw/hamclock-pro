#!/usr/bin/env python3
"""Settings: empty DX fields mean "no DX" (saved as DX = DE), never the old target or 0,0 ("Accra")."""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml"
settings = (root / "SettingsPage.qml").read_text()
for field in ("dxLat", "dxLon"):
    block = settings.split(f"id: {field};", 1)[1][:200]
    if "appSettings.hasDx" not in block:
        sys.exit(f"SettingsPage.qml: {field} must show empty when no DX is set")
save = settings.split("function save()", 1)[1].split("function", 1)[0]
if not re.search(r"dxTyped\s*\?[^:]+:\s*de\b", save):
    sys.exit("SettingsPage.qml: empty DX fields must save DX = DE (no DX)")
card = (root / "StationCard.qml").read_text()
if "station.hasDx" not in card:
    sys.exit("StationCard.qml: the DX column must hide when no DX is set")
print("ok")
