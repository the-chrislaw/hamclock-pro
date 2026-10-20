#!/bin/sh
# Fails when a statically linked Qt Quick plugin is missing from an iOS
# executable: the app would start and then show nothing (issue #389, #521).
# usage: check_ios_plugins.sh <path/to/hamclock-pro binary>
set -u
bin="${1:?usage: check_ios_plugins.sh <binary>}"
[ -f "$bin" ] || { echo "no such binary: $bin"; exit 2; }
syms=$(xcrun nm -arch arm64 "$bin" 2>/dev/null || nm "$bin") || exit 2
missing=0
for p in QtQuick2Plugin QtQuickControls2Plugin QtQuickControls2BasicStylePlugin \
         QtQuickLayoutsPlugin QtQuickWindowPlugin; do
  if printf '%s\n' "$syms" | grep -q "qt_static_plugin_$p"; then
    echo "ok: $p"
  else
    echo "MISSING static plugin: $p"
    missing=1
  fi
done
exit $missing
