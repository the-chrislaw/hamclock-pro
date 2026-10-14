#!/usr/bin/env python3
"""Every widget registered in src/widgets must have a pane chooser category."""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent
ids = set()
for p in (root / "src" / "widgets").rglob("*.cpp"):
    ids.update(re.findall(r'add\(\{"([a-z0-9]+)",\s*"', p.read_text()))
qml = (root / "src/ui/qml/PaneChooser.qml").read_text()
block = re.search(r"categoryOf: \(\{(.*?)\}\)", qml, re.S).group(1)
known = set(re.findall(r"([a-z0-9]+):\s*\d", block))
missing = sorted(ids - known)
if not ids:
    sys.exit("no widget ids found")
if missing:
    sys.exit("widgets without a pane category: " + ", ".join(missing))
print("ok", len(ids), "widgets categorised")
