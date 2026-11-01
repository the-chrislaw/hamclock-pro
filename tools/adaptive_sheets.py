#!/usr/bin/env python3
"""Contact sheets of every widget in pane 1 at several window sizes (adaptive panes).

Renders each widget with `hamclock-pro --screenshot --check-fill 0`, crops pane 1
(and the spill pane when one appears), and tiles the crops into one sheet per
size and theme, labelled with the widget id and the largest blank band the app
measured in the card body. Needs Pillow.

  tools/adaptive_sheets.py --app build-mac/hamclock-pro.app/Contents/MacOS/hamclock-pro \
      --out /tmp/sheets --sizes 1024x600,1920x1080 --themes dark,light [--fixtures]
"""
import argparse
import concurrent.futures
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_SIZES = "800x480,1024x600,1280x800,1920x1080,2560x1440,834x1210,1210x834,390x844"
# Public sample station (ARRL headquarters, W1AW) with a Tokyo DX.
STATION = "version=1\nde_lat=41.7292\nde_lon=-72.7083\ndx_lat=35.68\ndx_lon=139.69\ncallsign=W1AW\nappearance=dark\n"
FILL = re.compile(r'PANE FILL: slot (\d+) "(.*)" (\S+) at (-?\d+),(-?\d+) (\d+)x(\d+): blank (\d+)%')
TILE_WIDTH = 300
LABEL_HEIGHT = 18
RENDER_TIMEOUT_S = 60


def widget_ids():
    # Widget directories have a card (watchlist is shared code without one).
    ids = sorted(p.name for p in (ROOT / "src/widgets").iterdir() if p.is_dir() and any(p.glob("*Card.qml")))
    return ids + ["passes"]


def render(app, widget, size, theme, use_fixtures, work, cache, slot):
    config = Path(tempfile.mkdtemp(dir=work))
    (config / "station.conf").write_text(STATION)
    png = config / "shot.png"
    command = [app, "--config-dir", str(config), "--theme", theme, "--size", size, "--pane", f"{slot}={widget}",
               "--screenshot", str(png), "--allow-truncation", "--check-fill", "0"]
    fixtures = ROOT / "src/widgets" / ("satellites" if widget == "passes" else widget) / "tests/fixtures"
    if use_fixtures and fixtures.is_dir():
        command += ["--fixtures", str(fixtures)]
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", XDG_CACHE_HOME=str(cache / widget))
    try:
        run = subprocess.run(command, capture_output=True, text=True, env=env, timeout=RENDER_TIMEOUT_S)
        output = run.stdout + run.stderr
    except subprocess.TimeoutExpired:
        output = "timed out"
    panes = [m.groups() for m in FILL.finditer(output)]
    truncated = [line for line in output.splitlines() if line.startswith("TRUNCATED")]
    try:
        image = Image.open(png).convert("RGB")
    except OSError:  # no screenshot (crash or timeout)
        image = None
    shutil.rmtree(config, ignore_errors=True)
    return widget, image, panes, truncated


def crops(image, panes, slot):
    """The pane the widget was put in."""
    picked = [p for p in panes if p[0] == str(slot)]
    out = []
    for slot, title, state, x, y, w, h, blank in picked:
        box = (int(x), int(y), int(x) + int(w), int(y) + int(h))
        out.append((image.crop(box), f"{title} {state} {w}x{h} blank {blank}%"))
    return out


def sheet(results, path, title, slot):
    font = ImageFont.load_default()
    tiles = []
    for widget, image, panes, truncated in results:
        if image is None:
            continue
        for crop, label in crops(image, panes, slot):
            scale = TILE_WIDTH / crop.width
            tile = crop.resize((TILE_WIDTH, max(1, int(crop.height * scale))))
            tiles.append((tile, f"{widget}: {label}" + (" TRUNC" if truncated else "")))
    if not tiles:
        return
    columns = 8 if len(tiles) > 24 else 6
    row_height = max(t.height for t, _ in tiles) + LABEL_HEIGHT
    rows = (len(tiles) + columns - 1) // columns
    canvas = Image.new("RGB", (columns * (TILE_WIDTH + 6), rows * row_height + 24), (40, 40, 40))
    draw = ImageDraw.Draw(canvas)
    draw.text((6, 4), title, fill=(255, 255, 255), font=font)
    for i, (tile, label) in enumerate(tiles):
        x = (i % columns) * (TILE_WIDTH + 6)
        y = 24 + (i // columns) * row_height
        draw.text((x + 2, y + 2), label[:56], fill=(255, 220, 120), font=font)
        canvas.paste(tile, (x, y + LABEL_HEIGHT))
    canvas.save(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--app", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--sizes", default=DEFAULT_SIZES)
    parser.add_argument("--themes", default="dark")
    parser.add_argument("--widgets", default="")
    parser.add_argument("--fixtures", action="store_true", help="serve each widget's test fixtures (deterministic)")
    parser.add_argument("--jobs", type=int, default=8)
    parser.add_argument("--keep", action="store_true", help="also save each full screenshot")
    parser.add_argument("--slot", type=int, default=1, help="pane number to put each widget in (e.g. the spill pane)")
    args = parser.parse_args()

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="sheets-"))
    cache = out / "cache"
    widgets = args.widgets.split(",") if args.widgets else widget_ids()
    report = []
    suffix = "" if args.slot == 1 else f"-slot{args.slot}"
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        for theme in args.themes.split(","):
            for size in args.sizes.split(","):
                jobs = [pool.submit(render, args.app, w, size, theme, args.fixtures, work, cache, args.slot) for w in widgets]
                results = [j.result() for j in jobs]
                sheet(results, out / f"sheet-{size}-{theme}{suffix}.png", f"{size} {theme} pane {args.slot}", args.slot)
                for widget, image, panes, truncated in results:
                    if args.keep and image is not None:
                        image.save(out / f"{widget}-{size}-{theme}{suffix}.png")
                    pane = next((p for p in panes if p[0] == str(args.slot)), None)
                    report.append(f"{size}\t{theme}\t{widget}\t{pane[2] if pane else '?'}\t"
                                  f"{pane[7] if pane else '?'}\t{'; '.join(truncated)}")
                print(f"{size} {theme} done", file=sys.stderr)
    (out / f"report{suffix}.tsv").write_text("size\ttheme\twidget\tstate\tblank%\ttruncated\n" + "\n".join(report) + "\n")
    shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
