#pragma once
// Callsign banner modes: tap cycles call -> "ON AIR" -> "73 DE <call>" -> call.
#include <cmath>
#include <QString>

namespace hamclock {

constexpr int kBannerModes = 3;

inline int next_banner_mode(int mode) { return ((mode % kBannerModes) + kBannerModes + 1) % kBannerModes; }

inline bool banner_is_callsign(int mode) { return ((mode % kBannerModes) + kBannerModes) % kBannerModes == 0; }

inline QString banner_text(int mode, const QString& call) {
    const QString c = call.trimmed().toUpper();
    switch (((mode % kBannerModes) + kBannerModes) % kBannerModes) {
    case 1: return QStringLiteral("ON AIR");
    case 2: return c.isEmpty() ? QStringLiteral("73") : QStringLiteral("73 DE ") + c;
    default: return c;
    }
}

// Banner colour styles (Settings > Callsign banner): 0 theme accent, 1 amber, 2 green,
// 3 red, 4 white, 5 rainbow (hue turns once per 12 s). Returns "" for the theme accent.
constexpr int kBannerStyles = 6;

inline QString banner_color(int style, qint64 secs) {
    switch (style) {
    case 1: return QStringLiteral("#ffb000");
    case 2: return QStringLiteral("#3ddc84");
    case 3: return QStringLiteral("#ff4d4d");
    case 4: return QStringLiteral("#ffffff");
    case 5: {
        // HSL(h, 0.9, 0.6) -> RGB.
        const double h = static_cast<double>(((secs % 12) + 12) % 12) / 12.0 * 6.0;
        const double c = (1.0 - std::fabs(2 * 0.6 - 1.0)) * 0.9, x = c * (1.0 - std::fabs(std::fmod(h, 2.0) - 1.0));
        const double m = 0.6 - c / 2;
        double r = 0, g = 0, b = 0;
        switch (static_cast<int>(h)) {
        case 0: r = c; g = x; break;
        case 1: r = x; g = c; break;
        case 2: g = c; b = x; break;
        case 3: g = x; b = c; break;
        case 4: r = x; b = c; break;
        default: r = c; b = x; break;
        }
        return QStringLiteral("#%1%2%3")
            .arg(static_cast<int>((r + m) * 255 + 0.5), 2, 16, QLatin1Char('0'))
            .arg(static_cast<int>((g + m) * 255 + 0.5), 2, 16, QLatin1Char('0'))
            .arg(static_cast<int>((b + m) * 255 + 0.5), 2, 16, QLatin1Char('0'));
    }
    default: return {};
    }
}

}  // namespace hamclock
