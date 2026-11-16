#!/usr/bin/env python3
"""The app lays out the phone sizes, Settings and a rotation without layout or binding loops.

Qt reports "Layout polish loop detected", "Binding loop detected" and "Detected recursive
rearrange" when a layout's size keeps feeding back into itself. A loop like that hung the
iPhone app on a white screen at launch (#900); others left Settings half laid out after a
rotation. Runs the real app offscreen (--offline, --screenshot) at 390x844 and 844x390, with
Settings open at several sections or the pane chooser open (--chooser), and turned from one
to the other (--rotate-from), and fails on any such message, or on a QML context error, in its output.

usage: qml_layout_loops_regression.py <hamclock-pro binary> <station.conf>
"""
from __future__ import annotations

import re
import shutil
import subprocess
import sys
import tempfile

LOOP = re.compile(r"polish loop|binding loop|recursive rearrange|invalid context", re.I)
PORTRAIT, LANDSCAPE = "390x844", "844x390"
# (size, rotated from, settings section or None)
RUNS = [(size, None, section) for size in (PORTRAIT, LANDSCAPE)
        for section in (None, "accounts:dxcluster", "display", "spots", "advanced")]
RUNS += [(to, start, section) for start, to in ((PORTRAIT, LANDSCAPE), (LANDSCAPE, PORTRAIT))
         for section in (None, "station", "accounts:dxcluster", "display", "spots")]
# The pane chooser: section "chooser" opens it for pane 1 instead of Settings.
CHOOSER = "chooser"
RUNS += [(size, start, CHOOSER) for size, start in ((PORTRAIT, None), (LANDSCAPE, None), (LANDSCAPE, PORTRAIT))]
TIMEOUT_S = 60


def run(binary: str, station: str, size: str, start: str | None, section: str | None) -> list[str]:
    with tempfile.TemporaryDirectory() as config:
        # A copy: the app may save the station (a time zone picked from the location) back to it.
        station_copy = shutil.copy(station, f"{config}/station.conf")
        args = [binary, "--offline", "--config-dir", config, "--station", station_copy, "--size", size,
                "--screenshot", f"{config}/shot.png", "--allow-truncation"]
        if start:
            args += ["--rotate-from", start]
        if section == CHOOSER:
            args += ["--chooser", "1"]
        elif section:
            args += ["--settings", section]
        result = subprocess.run(args, capture_output=True, text=True, timeout=TIMEOUT_S)
    output = result.stdout + result.stderr
    problems = sorted({line.strip() for line in output.splitlines() if LOOP.search(line)})
    if result.returncode != 0:
        problems.append(f"exit code {result.returncode}")
    return problems


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    binary, station = sys.argv[1], sys.argv[2]
    failed = 0
    for size, start, section in RUNS:
        shown = "pane chooser" if section == CHOOSER else f"Settings {section or 'closed'}"
        label = f"{start + ' -> ' if start else ''}{size}, {shown}"
        problems = run(binary, station, size, start, section)
        if problems:
            failed += 1
            print(f"FAIL {label}:")
            for problem in problems:
                print("   ", problem)
    if failed:
        return 1
    print(f"ok: {len(RUNS)} runs without layout or binding loops")
    return 0


if __name__ == "__main__":
    sys.exit(main())
