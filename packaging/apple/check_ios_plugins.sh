#!/bin/sh
# Fails when a statically linked Qt Quick plugin is missing from an iOS
# executable: the app would start and then show nothing (issue #389, #521).
# usage: check_ios_plugins.sh <path/to/hamclock-pro binary>
set -u
bin="${1:?usage: check_ios_plugins.sh <binary>}"
[ -f "$bin" ] || { echo "no such binary: $bin"; exit 2; }
syms=$(mktemp)
xcrun nm -arch arm64 "$bin" > "$syms" 2>/dev/null || nm "$bin" > "$syms" || exit 2
missing=0
# QtQuick.Window lives inside the QtQuick plugin in Qt 6.5 (no separate symbol).
echo "linked static plugins:"; grep -Eo "qt_static_plugin_[A-Za-z0-9_]+" "$syms" | sort -u
for p in QtQuick2Plugin QtQuickControls2Plugin QtQuickControls2BasicStylePlugin \
         QtQuickLayoutsPlugin; do
  if grep -Eq "qt_static_plugin_$p" "$syms"; then
    echo "ok: $p"
  else
    echo "MISSING static plugin: $p"
    missing=1
  fi
done
rm -f "$syms"
exit $missing
