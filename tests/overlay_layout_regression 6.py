#!/usr/bin/env python3
"""Full-screen pages (Settings, Timers, Big Clock) own the whole window.

1. The window header (title, Show/Hide path, Settings) sits outside the content
   item, so the overlay layers cannot cover it. It must hide while an overlay is
   open, or it stays on top of Settings and the Big Clock with buttons that do
   nothing (the page underneath is input-blocked).
2. Big Clock: the UTC caption belongs to the UTC digits. In a tall window (iPad or
   phone portrait) the digits fit the width and sit in the middle of a tall box,
   so they must be bottom-aligned or the caption floats next to the local clock.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent / "src/ui/qml"


def header_hides_for_overlays(main: str) -> bool:
    m = re.search(r"header:\s*Rectangle\s*\{(.*?)\n\s*RowLayout", main, re.S)
    return bool(m) and re.search(r"visible:.*!window\.overlayOpen", m.group(1)) is not None


def utc_digits_bottom_aligned(big: str) -> bool:
    m = re.search(r"Text\s*\{((?:(?!Text\s*\{).)*?text:\s*station\.utcTime\b.*?)\n\s{8}\}", big, re.S)
    return bool(m) and "verticalAlignment: Text.AlignBottom" in m.group(1)


main = (ROOT / "Main.qml").read_text()
big = (ROOT / "BigClockPage.qml").read_text()
if not header_hides_for_overlays(main):
    sys.exit("Main.qml: the window header must hide while Settings, Timers or Big Clock is open")
if not utc_digits_bottom_aligned(big):
    sys.exit("BigClockPage.qml: UTC digits must sit on their caption (verticalAlignment: Text.AlignBottom)")
print("ok")
