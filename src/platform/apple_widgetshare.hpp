#pragma once
// Station hand-off to the iPhone/iPad home-screen widgets (apple/widgets). The
// widget extension cannot read the app's own container, so on iOS the app
// copies the few fields the widgets need (callsign, DE position, tracked
// satellites) into the shared App Group defaults. Building the payload is
// platform-free and tested on Linux; only the write is iOS code. The key names
// are a contract with apple/widgets/Shared/AppGroup.swift; change both together.
#include "settings.hpp"
#include <QStringList>
#include <QVariantMap>

namespace hamclock {

inline constexpr char kWidgetAppGroup[] = "group.com.chrislaw.hamclockpro";
inline constexpr int kWidgetShareVersion = 1;
inline constexpr int kWidgetMaxTracked = 2;      // the app tracks at most two (satellites_model)
inline constexpr int kWidgetMaxNameLength = 32;

struct WidgetShare {
    bool configured = false;  // false: the widgets show "Open HamClock Pro to set your station"
    QString callsign;
    double latitude = 0;
    double longitude = 0;
    QStringList tracked;      // satellite short names, as in [satellites] tracked=
};

// Validates and trims: an unconfigured station or an out-of-range position gives
// configured = false (callsign and position cleared); callsign upper-cased; tracked
// names trimmed, de-duplicated, empty/overlong ones dropped, at most kWidgetMaxTracked.
WidgetShare makeWidgetShare(const StationSettings& station, bool configured, const QStringList& tracked);

// The key/value pairs written to the App Group defaults ("station.callsign", ...),
// including "share.version" and "share.updated" (unix seconds, from now_s).
QVariantMap widgetShareFields(const WidgetShare& share, qint64 now_s);

// Writes the fields to NSUserDefaults(suiteName: kWidgetAppGroup) on iOS and
// returns true; does nothing and returns false elsewhere.
bool nativeShareForWidgets(const WidgetShare& share);

} // namespace hamclock
