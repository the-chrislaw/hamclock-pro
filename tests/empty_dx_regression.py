#!/usr/bin/env python3
"""Settings: no DX target (saved as DX = DE) is never shown or saved as the old target or 0,0 ("Accra").

Since #702 item 8 DX is set on the map (or by grid in Advanced), not typed as coordinates in the
Station card, so the checks follow the read-only DX line and the station save.
"""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml"
settings = (root / "SettingsPage.qml").read_text()
if re.search(r"id:\s*dx(Lat|Lon)\b", settings):
    sys.exit("SettingsPage.qml: DX is not typed as coordinates in the Station card any more")
dx_line = settings.split('text: "DX target"', 1)[1][:400]
if "appSettings.hasDx" not in dx_line:
    sys.exit("SettingsPage.qml: the read-only DX line must say when no DX is set")
save = settings.split("function save()", 1)[1].split("function", 1)[0]
if not re.search(r"appSettings\.hasDx\s*\?[^:]+:\s*de\b", save):
    sys.exit("SettingsPage.qml: with no DX target, saving the station keeps DX = DE (no DX)")
card = (root / "StationCard.qml").read_text()
if "station.hasDx" not in card:
    sys.exit("StationCard.qml: the DX column must hide when no DX is set")
print("ok")
