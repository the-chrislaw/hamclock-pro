#!/usr/bin/env python3
"""Focus rings show for keyboard and remote focus only, never on a control that was just tapped.

On iPhone a tapped button or pane kept a bright focus ring, because every ring followed
activeFocus, which a tap also gives. A ring must also require Theme.keyboardFocus, which
main.cpp keeps in step with FocusVisibility (true after a key press until the next tap or
click). Text fields keep their own focus border: it shows where typing goes.
"""
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
# Text fields: the border marks where typing goes, so it follows activeFocus alone.
TEXT_FIELD_RINGS = {"src/ui/qml/FormField.qml", "src/ui/qml/TimeZonePicker.qml"}
KEYBOARD_ONLY = re.compile(r"Theme\.keyboardFocus|focusRing")


def main() -> int:
    errors: list[str] = []
    for path in sorted((ROOT / "src").rglob("*.qml")):
        rel = path.relative_to(ROOT).as_posix()
        if rel in TEXT_FIELD_RINGS:
            continue
        for number, line in enumerate(path.read_text().splitlines(), 1):
            if "Theme.focus" in line and not KEYBOARD_ONLY.search(line):
                errors.append(f"{rel}:{number}: focus ring without Theme.keyboardFocus: {line.strip()}")
    theme = (ROOT / "src/ui/qml/Theme.qml").read_text()
    if not re.search(r"property bool keyboardFocus\b", theme):
        errors.append("Theme.qml: needs a keyboardFocus property for FocusVisibility to set")
    main_cpp = (ROOT / "src/ui/main.cpp").read_text()
    if not re.search(r'mirrorTo\([^;]*"keyboardFocus"\)', main_cpp) or "install(&app)" not in main_cpp:
        errors.append("main.cpp: FocusVisibility must watch the app and keep Theme.keyboardFocus in step")
    for error in errors:
        print("FAIL:", error)
    if errors:
        return 1
    print("ok: focus rings follow the keyboard only")
    return 0


if __name__ == "__main__":
    sys.exit(main())
