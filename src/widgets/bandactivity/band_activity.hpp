#pragma once
// Band activity: how many recent spots fall on each band, split by the continent of the spotted station.
#include <QString>
#include <QStringList>
#include <functional>
#include <map>
#include <vector>

namespace hamclock::bandactivity {

inline const QStringList& continents() {
    static const QStringList c{"NA", "SA", "EU", "AF", "AS", "OC"};
    return c;
}

struct BandRow {
    QString band;
    int total = 0;
    std::map<QString, int> by_continent; // only continents with spots; unknown entities count in `total` only
};

// `spots` is any range of {band, dx}; `continent_of(call)` returns "" when unknown. Rows follow `band_order`
// (bands without spots are left out) so the table keeps its shape between updates.
template <class Range>
std::vector<BandRow> summarize(const Range& spots, const QStringList& band_order,
                               const std::function<QString(const QString&)>& continent_of) {
    std::map<QString, BandRow> rows;
    for (const auto& s : spots) {
        if (s.band.isEmpty()) continue;
        BandRow& r = rows[s.band];
        r.band = s.band;
        ++r.total;
        const QString c = continent_of ? continent_of(s.dx) : QString();
        if (continents().contains(c)) ++r.by_continent[c];
    }
    std::vector<BandRow> out;
    for (const auto& b : band_order) {
        auto it = rows.find(b);
        if (it != rows.end()) out.push_back(it->second);
    }
    return out;
}

} // namespace hamclock::bandactivity
