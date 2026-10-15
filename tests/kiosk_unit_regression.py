#!/usr/bin/env python3
"""Offline sanity check of the kiosk systemd unit (no systemd needed)."""
import configparser
import pathlib
import sys

unit = pathlib.Path(__file__).resolve().parent.parent / "src/ui/kiosk/hamclock-pro-kiosk.service"
cfg = configparser.ConfigParser(strict=False, interpolation=None)
cfg.optionxform = str
cfg.read_string(unit.read_text())
errors = []
for section in ("Unit", "Service", "Install"):
    if section not in cfg:
        errors.append("missing [%s]" % section)
if not errors:
    svc = cfg["Service"]
    if "--fullscreen" not in svc.get("ExecStart", ""):
        errors.append("ExecStart must pass --fullscreen")
    if svc.get("User", "root") == "root":
        errors.append("must not run as root")
    if svc.get("NoNewPrivileges") != "true":
        errors.append("NoNewPrivileges=true required")
    if "QT_QPA_PLATFORM=eglfs" not in unit.read_text():
        errors.append("eglfs platform not set")
    if cfg["Install"].get("WantedBy") != "multi-user.target":
        errors.append("WantedBy must be multi-user.target")
for e in errors:
    print("FAIL:", e)
sys.exit(1 if errors else 0)
