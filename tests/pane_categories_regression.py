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
# Every widget has a category, so "Other" is empty unless one is missing: it must not be forced on.
if re.search(r"index === 6\)\s*return true", qml) or "index === 0 || index === 6" in qml:
    sys.exit('the "Other" tab is always shown even though it has no widgets')
print("ok", len(ids), "widgets categorised")
