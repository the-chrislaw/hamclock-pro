#!/usr/bin/env python3
"""Map buttons (Map menu, zoom, reset) are opaque, so markers under them never show through."""
import pathlib, re, sys

main = (pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml/Main.qml").read_text()
translucent = re.findall(r'color:[^\n]*"#cc[0-9a-fA-F]{6}"', main)
if translucent:
    sys.exit("Main.qml: map buttons must be opaque, found " + "; ".join(t.strip() for t in translucent))
print("ok")
