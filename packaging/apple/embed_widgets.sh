#!/bin/sh
# Copies the WidgetKit extension (apple/widgets) into the Qt-built iOS app's
# PlugIns/ and checks what App Store validation and the system require of an
# embedded extension. Signing is separate (the TestFlight job signs the appex,
# then re-signs the app). Used by .github/workflows/apple.yml.
#
#   embed_widgets.sh <path/hamclock-pro.app> <path/HamClockWidgets.appex>
set -eu

APP=${1:?usage: embed_widgets.sh <app> <appex>}
APPEX=${2:?usage: embed_widgets.sh <app> <appex>}
PB=/usr/libexec/PlistBuddy
fail() { echo "embed_widgets: $*" >&2; exit 1; }

[ -f "$APP/Info.plist" ] || fail "no app at $APP"
[ -f "$APPEX/Info.plist" ] || fail "no extension at $APPEX"

app_id=$($PB -c 'Print :CFBundleIdentifier' "$APP/Info.plist")
ext_id=$($PB -c 'Print :CFBundleIdentifier' "$APPEX/Info.plist")
case "$ext_id" in
    "$app_id".*) ;;
    *) fail "extension id $ext_id is not prefixed by the app id $app_id" ;;
esac

point=$($PB -c 'Print :NSExtension:NSExtensionPointIdentifier' "$APPEX/Info.plist")
[ "$point" = com.apple.widgetkit-extension ] || fail "not a WidgetKit extension ($point)"

# App Store Connect rejects extensions whose versions differ from the app's.
for key in CFBundleShortVersionString CFBundleVersion; do
    a=$($PB -c "Print :$key" "$APP/Info.plist")
    e=$($PB -c "Print :$key" "$APPEX/Info.plist")
    [ "$a" = "$e" ] || fail "$key differs: app $a, extension $e"
done

exe=$($PB -c 'Print :CFBundleExecutable' "$APPEX/Info.plist")
lipo -archs "$APPEX/$exe" | grep -qw arm64 || fail "extension binary is not arm64"

mkdir -p "$APP/PlugIns"
rm -rf "$APP/PlugIns/$(basename "$APPEX")"
ditto "$APPEX" "$APP/PlugIns/$(basename "$APPEX")"
echo "embedded $ext_id ($($PB -c 'Print :CFBundleShortVersionString' "$APPEX/Info.plist")) into $app_id"
