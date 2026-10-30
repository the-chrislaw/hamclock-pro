#!/usr/bin/env python3
"""Phone pane swipe: runs tests/pane_swipe/tst_pane_swipe.qml against the real
PaneSlot.qml with stub settings and widgets (usage: pane_swipe_regression.py QMLTESTRUNNER).

The app's context objects (appSettings, widgetRegistry, blanking) are swapped for
the HamClock.Stubs singleton in a temporary copy of the QML."""
import os, pathlib, shutil, subprocess, sys, tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
QML = ROOT / "src/ui/qml"
HARNESS = ROOT / "tests/pane_swipe"
SLOT_FILES = ["PaneSlot.qml", "PageDots.qml", "pager.mjs", "Card.qml", "StatusBadge.qml", "AppButton.qml"]
STUBS = {"appSettings.": "Stubs.settings.", "widgetRegistry": "Stubs.registry", "blanking.touch": "Stubs.touch"}


def prepare(work: pathlib.Path) -> None:
    shutil.copytree(HARNESS, work, dirs_exist_ok=True)
    shutil.copy(QML / "Theme.qml", work / "HamClock" / "Theme.qml")
    for name in SLOT_FILES:
        text = (QML / name).read_text()
        for real, stub in STUBS.items():
            text = text.replace(real, stub)
        (work / name).write_text(text)


def main() -> None:
    if len(sys.argv) < 2:
        sys.exit("usage: pane_swipe_regression.py QMLTESTRUNNER")
    with tempfile.TemporaryDirectory() as tmp:
        work = pathlib.Path(tmp)
        prepare(work)
        env = dict(os.environ, QT_QPA_PLATFORM=os.environ.get("QT_QPA_PLATFORM", "offscreen"))
        run = subprocess.run([sys.argv[1], "-import", str(work), "-input", str(work / "tst_pane_swipe.qml")],
                             cwd=tmp, env=env, capture_output=True, text=True, timeout=120)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        sys.exit(run.returncode)


if __name__ == "__main__":
    main()
