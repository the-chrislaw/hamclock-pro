#!/usr/bin/env python3
"""Regenerates AppIcon.png (1024x1024): a clock ring with a hand on a dark tile.
Pure stdlib so it runs anywhere:  python3 make_icon_png.py AppIcon.png"""
import math, struct, sys, zlib

N = 1024
BG, RING, HAND = (16, 24, 38), (64, 200, 160), (240, 245, 250)

def px(x, y):
    cx = cy = N / 2
    # rounded-square mask
    r, m = 180, 40
    dx = max(abs(x - cx) - (cx - m - r), 0)
    dy = max(abs(y - cy) - (cy - m - r), 0)
    if math.hypot(dx, dy) > r:
        return (0, 0, 0, 0)
    d = math.hypot(x - cx, y - cy)
    if 300 <= d <= 350:
        return RING + (255,)
    for ang, ln, w in ((-90, 250, 16), (-30, 180, 16)):   # hands
        a = math.radians(ang)
        t = (x - cx) * math.cos(a) + (y - cy) * math.sin(a)
        s = abs(-(x - cx) * math.sin(a) + (y - cy) * math.cos(a))
        if 0 <= t <= ln and s <= w:
            return HAND + (255,)
    return BG + (255,)

raw = b"".join(b"\x00" + bytes(c for x in range(N) for c in px(x, y)) for y in range(N))
def chunk(t, d):
    c = struct.pack(">I", len(d)) + t + d
    return c + struct.pack(">I", zlib.crc32(t + d))
png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", N, N, 8, 6, 0, 0, 0)) \
    + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
open(sys.argv[1], "wb").write(png)
