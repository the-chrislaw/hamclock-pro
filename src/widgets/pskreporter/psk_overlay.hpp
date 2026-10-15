#pragma once
// Map overlay layer for PSKReporter receivers (see docs/map-overlays.md).
#include "../../map/overlay.hpp"
#include "psk_parse.hpp"
#include <QRegularExpression>
#include <QStringList>
#include <vector>

namespace hamclock::psk {

inline constexpr char kOverlayLayerId[] = "psk";

// One marker per receiving station (newest report wins, input is newest
// first), placed at the centre of its 4- or 6-character grid square with a
// short path from DE. Receivers without a usable grid are not drawn.
OverlayLayer overlay_layer(const std::vector<Report>& reports, int max_markers = 30);

// Farthest receiver from DE among reports with a 4/6-character grid (distance
// to the grid centre). found is false when no report has a usable grid.
struct Farthest {
    bool found = false;
    QString call, band;
    double km = 0;
};
Farthest farthest_report(const std::vector<Report>& reports, double de_lat, double de_lon);

// Reports whose receiver grid centre is within max_km of DE, order kept. max_km <= 0 keeps
// everything; with a limit set, reports without a usable grid are dropped (distance unknown).
std::vector<Report> within_distance(const std::vector<Report>& reports, double de_lat, double de_lon, double max_km);

// Normalises a comma/space separated filter ("VK, JA, PM95") to upper-case tokens
// (letters, digits and '/'), at most 8.
inline QStringList match_tokens(const QString& filter) {
    QStringList out;
    for (const QString& raw : filter.split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts)) {
        const QString t = raw.toUpper();
        if (t.size() > 12 || out.size() >= 8) continue;
        bool ok = true;
        for (const QChar c : t) ok = ok && ((c.isLetterOrNumber() && c.unicode() < 128) || c == '/');
        if (ok && !out.contains(t)) out << t;
    }
    return out;
}

// Reports whose receiver call or receiver grid starts with any token, order kept.
// An empty token list keeps everything.
std::vector<Report> matching(const std::vector<Report>& reports, const QStringList& tokens);

} // namespace hamclock::psk
