#!/usr/bin/env python3
"""Regenerates the tvOS brand assets (layered app icon + top shelf images) in
App/Resources/Assets.xcassets: the iOS clock-ring motif from
packaging/apple/make_icon_png.py on a dark map grid. Pure stdlib:
    python3 apple/tvos/make_tv_icons.py
The back layers are opaque RGB, as App Store validation requires; the front
layer (ring and hands) is RGBA so tvOS can parallax it over the back."""
import json, math, os, struct, zlib

BG, GRID, RING, HAND = (16, 24, 38), (30, 44, 64), (64, 200, 160), (240, 245, 250)
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "App", "Resources", "Assets.xcassets")
BRAND = "App Icon & Top Shelf Image.brandassets"


def cover(d):
    """Antialiased coverage for a signed distance d (negative = inside), in pixels."""
    return max(0.0, min(1.0, 0.5 - d))


def clock_alpha(x, y, cx, cy, r):
    """Coverage of the ring and the two hands for a clock of radius r at (cx, cy)."""
    d = math.hypot(x - cx, y - cy)
    a = cover(abs(d - 0.93 * r) - 0.07 * r)
    for ang, ln in ((-90, 0.71), (-30, 0.51)):
        t = math.radians(ang)
        along = (x - cx) * math.cos(t) + (y - cy) * math.sin(t)
        across = abs(-(x - cx) * math.sin(t) + (y - cy) * math.cos(t))
        a = max(a, cover(max(across - 0.046 * r, -along, along - ln * r)))
    return a


def is_ring(x, y, cx, cy, r):
    return abs(math.hypot(x - cx, y - cy) - 0.93 * r) <= 0.07 * r + 0.5


def back(x, y, w, h):
    step = h / 6
    on = min(x % step, step - x % step) < max(1.0, h / 400) or min(y % step, step - y % step) < max(1.0, h / 400)
    return GRID if on else BG


def png(path, w, h, pixel, alpha):
    rows = []
    for y in range(h):
        row = bytearray(b"\x00")
        for x in range(w):
            row += bytes(pixel(x + 0.5, y + 0.5)[: 4 if alpha else 3])
        rows.append(bytes(row))

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d))

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6 if alpha else 2, 0, 0, 0)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(b"".join(rows), 9))
                + chunk(b"IEND", b""))


def blend(a, b, t):
    return tuple(round(p + (q - p) * t) for p, q in zip(a, b))


def front(w, h):
    r = 0.4 * h

    def px(x, y):
        a = clock_alpha(x, y, w / 2, h / 2, r)
        col = RING if is_ring(x, y, w / 2, h / 2, r) else HAND
        return col + (round(255 * a),)
    return px


def shelf(w, h):
    r, cx = 0.32 * h, 0.2 * w

    def px(x, y):
        a = clock_alpha(x, y, cx, h / 2, r)
        col = RING if is_ring(x, y, cx, h / 2, r) else HAND
        return blend(back(x, y, w, h), col, a) + (255,)
    return px


def contents(path, obj):
    os.makedirs(path, exist_ok=True)
    with open(os.path.join(path, "Contents.json"), "w") as f:
        json.dump(obj, f, indent=2)
        f.write("\n")


INFO = {"author": "xcode", "version": 1}


def imageset(path, name, sizes, pixel_for, alpha):
    images = []
    for scale, (w, h) in sizes:
        fn = "%s@%dx.png" % (name, scale)
        png(os.path.join(path, fn), w, h, pixel_for(w, h), alpha)
        images.append({"filename": fn, "idiom": "tv", "scale": "%dx" % scale})
    contents(path, {"images": images, "info": INFO})


def imagestack(path, name, sizes):
    contents(path, {"info": INFO, "layers": [{"filename": "Front.imagestacklayer"},
                                             {"filename": "Back.imagestacklayer"}]})
    for layer, pixel_for, alpha in (("Front", front, True),
                                    ("Back", lambda w, h: lambda x, y: back(x, y, w, h) + (255,), False)):
        lp = os.path.join(path, layer + ".imagestacklayer")
        contents(lp, {"info": INFO})
        imageset(os.path.join(lp, "Content.imageset"), name + "-" + layer.lower(), sizes, pixel_for, alpha)


def main():
    contents(OUT, {"info": INFO})
    b = os.path.join(OUT, BRAND)
    contents(b, {"assets": [
        {"filename": "App Icon - App Store.imagestack", "idiom": "tv", "role": "primary-app-icon", "size": "1280x768"},
        {"filename": "App Icon.imagestack", "idiom": "tv", "role": "primary-app-icon", "size": "400x240"},
        {"filename": "Top Shelf Image Wide.imageset", "idiom": "tv", "role": "top-shelf-image-wide", "size": "2320x720"},
        {"filename": "Top Shelf Image.imageset", "idiom": "tv", "role": "top-shelf-image", "size": "1920x720"},
    ], "info": INFO})
    imagestack(os.path.join(b, "App Icon - App Store.imagestack"), "store", [(1, (1280, 768))])
    imagestack(os.path.join(b, "App Icon.imagestack"), "icon", [(1, (400, 240)), (2, (800, 480))])
    imageset(os.path.join(b, "Top Shelf Image.imageset"), "shelf", [(1, (1920, 720)), (2, (3840, 1440))], shelf, False)
    imageset(os.path.join(b, "Top Shelf Image Wide.imageset"), "shelf-wide",
             [(1, (2320, 720)), (2, (4640, 1440))], shelf, False)


if __name__ == "__main__":
    main()
