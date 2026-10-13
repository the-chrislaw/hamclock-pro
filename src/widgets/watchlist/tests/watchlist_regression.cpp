// Watch list matcher and its effect on the DX spot list, offline.
#include "../../dxcluster/dx_cluster_model.hpp"
#include "../watch_match.hpp"
#include <QCoreApplication>
#include <QTimeZone>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    const auto list = watchlist::sanitize({" k1abc ", "VP8*", "", "bad entry!", "K1ABC", "JA?AA", "x*********************"});
    check(list == QStringList({"K1ABC", "VP8*", "JA?AA"}), "sanitize uppercases, dedupes, drops invalid and over-long");
    check(watchlist::matches(list, "k1abc"), "exact call, case-insensitive");
    check(watchlist::matches(list, "K1ABC/P"), "portable suffix ignored");
    check(watchlist::matches(list, "EA8/K1ABC"), "operating prefix ignored");
    check(!watchlist::matches(list, "K1ABCD"), "exact entry is not a prefix");
    check(watchlist::matches(list, "VP8LP"), "prefix wildcard");
    check(watchlist::matches(list, "JA1AA"), "? matches one character");
    check(!watchlist::matches(list, "JA12AA"), "? does not match two");
    check(!watchlist::matches({}, "K1ABC"), "empty list matches nothing");
    check(!watchlist::matches(list, ""), "empty call matches nothing");

    DxSpotList spots;
    dxcluster::DxSpot a;
    a.dx = "VP8LP";
    a.time = QDateTime(QDate(2026, 10, 13), QTime(12, 0), QTimeZone::utc());
    dxcluster::DxSpot b = a;
    b.dx = "W1AW";
    spots.add(a);
    spots.add(b);
    check(spots.watchedCount() == 0, "no watch list, nothing watched");
    spots.set_watch(list);
    check(spots.watchedCount() == 1, "set_watch re-evaluates stored spots");
    check(spots.data(spots.index(1), DxSpotList::WatchedRole).toBool(), "older VP8 spot is watched");
    check(!spots.data(spots.index(0), DxSpotList::WatchedRole).toBool(), "newest W1AW is not");
    spots.add(a);
    check(spots.watchedCount() == 2, "new spots are checked on arrival");
    for (int i = 0; i < DxSpotList::kCapacity; ++i) spots.add(b);
    check(spots.watchedCount() == 0, "dropped spots leave the count");

    std::cerr << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
