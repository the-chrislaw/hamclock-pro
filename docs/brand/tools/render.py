#!/usr/bin/env python3
"""Renders the four HamClock Pro icon concepts and their review sheets into
docs/brand/concepts/. Needs resvg-py, Pillow and numpy:

    python3 -m venv /tmp/brandenv && /tmp/brandenv/bin/pip install resvg-py pillow numpy
    /tmp/brandenv/bin/python docs/brand/tools/render.py

The SVGs written next to each PNG are the source of truth; the PNGs are derived."""
import io
import os
import sys

import numpy as np
import resvg_py
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageOps

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import c_azimuth  # noqa: E402
import c_corona  # noqa: E402
import c_greyline  # noqa: E402
import c_skip  # noqa: E402
from svg import ART, compose  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "concepts")
CONCEPTS = [c_greyline.CONCEPT, c_azimuth.CONCEPT, c_skip.CONCEPT, c_corona.CONCEPT]
SMALL = (180, 120, 60, 40, 29)
TV_W, TV_H = 1280, 768
LIGHT_BG, DARK_BG = (242, 242, 247), (0, 0, 0)
INK, INK_DARK = (28, 28, 30), (235, 235, 245)
SUPERELLIPSE_N = 4.2  # close match to Apple's continuous-corner icon shape
FONT_PATHS = ("/System/Library/Fonts/SFNS.ttf", "/System/Library/Fonts/Helvetica.ttc")


def font(size):
    for path in FONT_PATHS:
        if os.path.exists(path):
            return ImageFont.truetype(path, size)
    return ImageFont.load_default(size)


def raster(svg, w, h):
    png = bytes(resvg_py.svg_to_bytes(svg_string=svg, width=w, height=h))
    return Image.open(io.BytesIO(png)).convert("RGBA")


def squircle_mask(size, supersample=4):
    n = size * supersample
    t = (np.arange(n) + 0.5) / n * 2 - 1
    xx, yy = np.meshgrid(t, t)
    inside = (np.abs(xx) ** SUPERELLIPSE_N + np.abs(yy) ** SUPERELLIPSE_N) <= 1.0
    img = Image.fromarray((inside * 255).astype(np.uint8), "L")
    return img.resize((size, size), Image.LANCZOS)


def masked(icon, size):
    """The icon as the OS shows it: scaled, squircle-masked, transparent outside."""
    out = icon.convert("RGB").resize((size, size), Image.LANCZOS).convert("RGBA")
    out.putalpha(squircle_mask(size))
    return out


def with_shadow(tile, blur_px, opacity=0.35):
    pad = blur_px * 3
    canvas = Image.new("RGBA", (tile.width + 2 * pad, tile.height + 2 * pad), (0, 0, 0, 0))
    shadow = Image.new("RGBA", tile.size, (0, 0, 0, int(255 * opacity)))
    shadow.putalpha(tile.getchannel("A").point(lambda a: int(a * opacity)))
    canvas.alpha_composite(shadow, (pad, pad + blur_px // 2))
    canvas = canvas.filter(ImageFilter.GaussianBlur(blur_px))
    canvas.alpha_composite(tile, (pad, pad))
    return canvas, pad


def save_svg(path, svg):
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(svg)


def render_variants(concept, folder):
    icons = {}
    for variant in ("light", "dark"):
        svg = compose(concept, variant)
        save_svg(os.path.join(folder, "icon.svg" if variant == "light" else "icon-dark.svg"), svg)
        icons[variant] = raster(svg, ART, ART).convert("RGB")
    icons["light"].save(os.path.join(folder, "icon-1024.png"))
    icons["dark"].save(os.path.join(folder, "icon-1024-dark.png"))
    fg = raster(compose(concept, "tinted", with_background=False), ART, ART)
    black = Image.new("RGBA", fg.size, (0, 0, 0, 255))
    black.alpha_composite(fg)
    tinted = ImageOps.autocontrast(black.convert("L"), cutoff=0.5)
    tinted.save(os.path.join(folder, "icon-1024-tinted.png"))
    icons["tinted"] = tinted.convert("RGB")
    return icons


def preview_masked(icons, folder):
    sheet = Image.new("RGB", (3 * 400 + 80, 470), LIGHT_BG)
    draw = ImageDraw.Draw(sheet)
    for i, key in enumerate(("light", "dark", "tinted")):
        tile, pad = with_shadow(masked(icons[key], 360), 14)
        sheet.paste(tile, (40 + i * 400 - pad, 30 - pad), tile)
        draw.text((40 + i * 400 + 180, 420), key.capitalize(), fill=INK, font=font(26), anchor="mm")
    sheet.save(os.path.join(folder, "preview-masked.png"))


def size_row(icon, bg, ink):
    width = sum(SMALL) + 40 * (len(SMALL) + 1)
    row = Image.new("RGB", (width, max(SMALL) + 70), bg)
    draw = ImageDraw.Draw(row)
    x = 40
    for s in SMALL:
        row.paste(masked(icon, s), (x, 20 + (max(SMALL) - s) // 2), masked(icon, s))
        draw.text((x + s // 2, max(SMALL) + 45), "%d px" % s, fill=ink, font=font(18), anchor="mm")
        x += s + 40
    return row


def small_sizes(icons, folder):
    rows = [size_row(icons["light"], LIGHT_BG, INK), size_row(icons["dark"], DARK_BG, INK_DARK)]
    sheet = Image.new("RGB", (rows[0].width, sum(r.height for r in rows)))
    y = 0
    for r in rows:
        sheet.paste(r, (0, y))
        y += r.height
    sheet.save(os.path.join(folder, "sizes.png"))


def rounded(img, radius):
    m = Image.new("L", img.size, 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, img.width - 1, img.height - 1), radius, fill=255)
    out = img.convert("RGBA")
    out.putalpha(m)
    return out


def tvos(concept, folder):
    names = [n for n, _ in concept.layers("light")]
    back_svg = compose(concept, "light", TV_W, TV_H, which=set(names[:-1]), k=concept.tv_scale)
    front_svg = compose(concept, "light", TV_W, TV_H, which={names[-1]}, k=concept.tv_scale,
                        with_background=False)
    save_svg(os.path.join(folder, "tvos-back.svg"), back_svg)
    save_svg(os.path.join(folder, "tvos-front.svg"), front_svg)
    back = raster(back_svg, TV_W, TV_H)
    front = raster(front_svg, TV_W, TV_H)
    back.convert("RGB").save(os.path.join(folder, "tvos-1280x768-back.png"))
    front.save(os.path.join(folder, "tvos-1280x768-front.png"))
    comp = back.copy()
    comp.alpha_composite(front)
    comp.convert("RGB").save(os.path.join(folder, "tvos-1280x768.png"))
    tile_w = 560
    tile_h = tile_w * TV_H // TV_W
    checker = Image.new("RGBA", (TV_W, TV_H), (58, 58, 64, 255))
    cd = ImageDraw.Draw(checker)
    for yy in range(0, TV_H, 48):
        for xx in range(0, TV_W, 48):
            if (xx // 48 + yy // 48) % 2:
                cd.rectangle((xx, yy, xx + 47, yy + 47), fill=(78, 78, 86, 255))
    checker.alpha_composite(front)
    shifted = back.copy()
    shifted.alpha_composite(front, (22, -12))
    panels = [("Back layer", back), ("Front layer", checker), ("Layered", comp), ("Parallax tilt", shifted)]
    sheet = Image.new("RGB", (2 * tile_w + 120, 2 * (tile_h + 70) + 30), (24, 24, 28))
    draw = ImageDraw.Draw(sheet)
    for i, (label, img) in enumerate(panels):
        x, y = 40 + (i % 2) * (tile_w + 40), 30 + (i // 2) * (tile_h + 70)
        tile = rounded(img.resize((tile_w, tile_h), Image.LANCZOS), 18)
        sheet.paste(tile, (x, y), tile)
        draw.text((x + tile_w // 2, y + tile_h + 26), label, fill=INK_DARK, font=font(22), anchor="mm")
    sheet.save(os.path.join(folder, "tvos-preview.png"))


GENERIC = [((88, 86, 214), "circle"), ((52, 199, 89), "bars"), ((255, 149, 0), "ring"),
           ((255, 59, 48), "square"), ((0, 122, 255), "tri"), ((142, 142, 147), "dots"),
           ((255, 204, 0), "line")]
LABELS = ["Photos", "HamClock", "Fitness", "Clock", "Music", "Maps", "Settings", "Notes"]


def generic_icon(color, glyph, size):
    img = Image.new("RGB", (size, size), color)
    d = ImageDraw.Draw(img)
    c, r, w = size // 2, size // 4, (255, 255, 255)
    if glyph == "circle":
        d.ellipse((c - r, c - r, c + r, c + r), fill=w)
    elif glyph == "bars":
        for i in range(3):
            d.rounded_rectangle((c - r + i * r * 0.75, c + r - (i + 1) * r * 0.6, c - r + i * r * 0.75 + r * 0.45,
                                 c + r), 6, fill=w)
    elif glyph == "ring":
        d.ellipse((c - r, c - r, c + r, c + r), outline=w, width=size // 14)
    elif glyph == "square":
        d.rounded_rectangle((c - r, c - r, c + r, c + r), size // 12, fill=w)
    elif glyph == "tri":
        d.polygon([(c - r, c + r * 0.8), (c + r, c + r * 0.8), (c, c - r)], fill=w)
    elif glyph == "dots":
        for i in (-1, 0, 1):
            d.ellipse((c + i * r - 14, c - 14, c + i * r + 14, c + 14), fill=w)
    else:
        d.rounded_rectangle((c - r, c - 10, c + r, c + 10), 10, fill=w)
    return img


def homescreen(icons, folder):
    size, gap_x, cols, rows = 192, 96, 4, 2
    w = cols * size + (cols + 1) * gap_x
    h = rows * (size + 80) + 120
    grad = np.linspace(0, 1, h)[:, None]
    top, bottom = np.array([255, 170, 120]), np.array([92, 72, 190])
    wall = (top * (1 - grad) + bottom * grad)[:, None, :].repeat(w, axis=1).reshape(h, w, 3)
    sheet = Image.fromarray(wall.astype(np.uint8), "RGB").convert("RGBA")
    draw = ImageDraw.Draw(sheet)
    generic = iter(GENERIC)
    for i, label in enumerate(LABELS):
        x = gap_x + (i % cols) * (size + gap_x)
        y = 60 + (i // cols) * (size + 80)
        src = icons["light"] if label == "HamClock" else generic_icon(*next(generic), ART // 4)
        tile, pad = with_shadow(masked(src, size), 10, 0.25)
        sheet.alpha_composite(tile, (x - pad, y - pad))
        draw.text((x + size // 2, y + size + 30), label, fill=(255, 255, 255), font=font(30), anchor="mm")
    sheet.convert("RGB").save(os.path.join(folder, "homescreen.png"))


def overview(all_icons):
    col, big = 460, 340
    smalls = (120, 60, 40, 29)
    band_h = 70 + big + 40 + max(smalls) + 50
    sheet = Image.new("RGB", (col * len(CONCEPTS) + 40, 2 * band_h + 20), LIGHT_BG)
    draw = ImageDraw.Draw(sheet)
    sheet.paste(DARK_BG, (0, band_h + 20, sheet.width, sheet.height))
    for band, (key, ink) in enumerate((("light", INK), ("dark", INK_DARK))):
        y0 = 20 + band * (band_h + 20)
        for i, concept in enumerate(CONCEPTS):
            x0 = 20 + i * col
            draw.text((x0 + col // 2, y0 + 24), concept.title + ("" if band == 0 else "  (dark)"),
                      fill=ink, font=font(30), anchor="mm")
            tile = masked(all_icons[concept.slug][key], big)
            sheet.paste(tile, (x0 + (col - big) // 2, y0 + 60), tile)
            x = x0 + (col - (sum(smalls) + 24 * (len(smalls) - 1))) // 2
            ys = y0 + 60 + big + 40
            for s in smalls:
                t = masked(all_icons[concept.slug][key], s)
                sheet.paste(t, (x, ys + (max(smalls) - s) // 2), t)
                x += s + 24
    sheet.save(os.path.join(OUT, "overview.png"))


def main():
    all_icons = {}
    for concept in CONCEPTS:
        folder = os.path.join(OUT, concept.slug)
        os.makedirs(folder, exist_ok=True)
        icons = render_variants(concept, folder)
        preview_masked(icons, folder)
        small_sizes(icons, folder)
        tvos(concept, folder)
        homescreen(icons, folder)
        all_icons[concept.slug] = icons
        print("rendered", concept.slug)
    overview(all_icons)
    print("wrote", os.path.normpath(os.path.join(OUT, "overview.png")))


if __name__ == "__main__":
    main()
