#!/usr/bin/env python3
"""Every QML file that uses the Overlay attached type must import QtQuick.Controls.

On Qt 6.4 (Ubuntu CI, Raspberry Pi OS bookworm) importing only QtQuick.Controls.Basic
leaves Overlay undefined, so the app fails at runtime with "Overlay is not defined"."""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent
missing = [str(p.relative_to(root)) for p in sorted(root.glob("src/**/*.qml"))
           if re.search(r"\bOverlay\.", p.read_text(encoding="utf-8"))
           and not re.search(r"^import QtQuick\.Controls\s*$", p.read_text(encoding="utf-8"), re.M)]
for path in missing:
    print(f"FAIL: {path} uses Overlay but does not import QtQuick.Controls")
sys.exit(1 if missing else 0)
