#!/usr/bin/env python3
"""A Canvas drawn before its pane has its final size can be smaller than its margin. A negative
radius makes ctx.arc() throw ("Incorrect argument radius"), which stops the paint, logs a QML error
and fails a --screenshot run (exit 3). Seen at random on the Moon disc at 1180x820 and 390x844.
Source checks, like the other QML regressions in tests/.
"""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent / "src"
for rel in ("widgets/sunmoon/SunMoonCard.qml", "widgets/satellites/SatellitesCard.qml",
            "widgets/rotator/RotatorCard.qml", "ui/qml/BigClockPage.qml"):
    text = (root / rel).read_text()
    match = re.search(r"\b(?:const|var) r = Math\.min\(width, height\) / 2[^\n]*\n\s*if \(r <= 0\) return", text)
    if not match:
        sys.exit(f"{rel}: onPaint must return when the radius is not positive, before any ctx.arc()")
    if text.index("ctx.arc(") < match.end():
        sys.exit(f"{rel}: the radius check must come before the first ctx.arc()")
print("ok")
