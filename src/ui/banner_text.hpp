#pragma once
// Callsign banner modes: tap cycles call -> "ON AIR" -> "73 DE <call>" -> call.
#include <QString>

namespace hamclock {

constexpr int kBannerModes = 3;

inline int next_banner_mode(int mode) { return ((mode % kBannerModes) + kBannerModes + 1) % kBannerModes; }

inline QString banner_text(int mode, const QString& call) {
    const QString c = call.trimmed().toUpper();
    switch (((mode % kBannerModes) + kBannerModes) % kBannerModes) {
    case 1: return QStringLiteral("ON AIR");
    case 2: return c.isEmpty() ? QStringLiteral("73") : QStringLiteral("73 DE ") + c;
    default: return c;
    }
}

}  // namespace hamclock
