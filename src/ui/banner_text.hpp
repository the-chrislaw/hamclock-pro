#pragma once
// Callsign banner modes: tap cycles call -> "ON AIR" -> "73 DE <call>" -> call.
#include <cmath>
#include <QString>

namespace hamclock {

constexpr int kBannerModes = 3;
constexpr int kBannerTitleMax = 24;

// With a custom title set (Settings > Callsign banner) the cycle gains a fourth step showing it.
inline int banner_mode_count(const QString& title) { return title.trimmed().isEmpty() ? kBannerModes : kBannerModes + 1; }

inline int banner_wrap(int mode, const QString& title) {
    const int n = banner_mode_count(title);
    return ((mode % n) + n) % n;
}

inline int next_banner_mode(int mode, const QString& title = {}) { return banner_wrap(mode + 1, title); }

inline bool banner_is_callsign(int mode, const QString& title = {}) { return banner_wrap(mode, title) == 0; }

inline QString banner_text(int mode, const QString& call, const QString& title = {}) {
    const QString c = call.trimmed().toUpper();
    switch (banner_wrap(mode, title)) {
    case 1: return QStringLiteral("ON AIR");
    case 2: return c.isEmpty() ? QStringLiteral("73") : QStringLiteral("73 DE ") + c;
    case 3: return title.trimmed().left(kBannerTitleMax);
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

// Banner background styles: 0 none (card colour), 1 black, 2 navy, 3 maroon, 4 white.
constexpr int kBannerBackgrounds = 5;

inline QString banner_background(int style) {
    switch (style) {
    case 1: return QStringLiteral("#000000");
    case 2: return QStringLiteral("#12306b");
    case 3: return QStringLiteral("#6b1a1a");
    case 4: return QStringLiteral("#ffffff");
    default: return {};
    }
}

}  // namespace hamclock
