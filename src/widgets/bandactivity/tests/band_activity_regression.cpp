// Band activity summary and its exposure on the DX cluster model, offline.
#include "../../dxcluster/dx_cluster_model.hpp"
#include "../band_activity.hpp"
#include <QCoreApplication>
#include <QTimeZone>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
QString fake_continent(const QString& call) {
    if (call.startsWith("W")) return "NA";
    if (call.startsWith("DL")) return "EU";
    if (call.startsWith("JA")) return "AS";
    return QString();
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    const QStringList order{"40m", "20m", "15m"};
    auto spot = [](const char* dx, const char* band) {
        dxcluster::DxSpot s;
        s.dx = dx;
        s.band = band;
        s.time = QDateTime(QDate(2026, 10, 14), QTime(12, 0), QTimeZone::utc());
        return s;
    };
    const std::vector<dxcluster::DxSpot> spots{spot("W1AW", "20m"), spot("DL1ABC", "20m"), spot("JA1AA", "40m"),
                                               spot("XX9XX", "20m"), spot("W2AA", ""), spot("DL2XYZ", "30m")};
    const auto rows = bandactivity::summarize(spots, order, fake_continent);
    check(rows.size() == 2, "only bands with spots, spots without a band or outside the order are skipped");
    check(rows[0].band == "40m" && rows[1].band == "20m", "rows follow the band order");
    check(rows[1].total == 3, "unknown-continent spot counts in the total");
    check(rows[1].by_continent.at("NA") == 1 && rows[1].by_continent.at("EU") == 1 && rows[1].by_continent.count("AS") == 0,
          "continent split");
    check(bandactivity::summarize(std::vector<dxcluster::DxSpot>{}, order, fake_continent).empty(), "no spots, no rows");
    check(bandactivity::summarize(spots, order, nullptr)[0].by_continent.empty(), "no resolver, totals only");

    DxSpotList list;
    for (const auto& s : spots) list.add(s);
    const auto act = list.activity(fake_continent);
    check(act.size() == 3, "list activity: 40m, 20m, 30m rows");
    check(act[0].toMap().value("band") == "40m" && act[2].toMap().value("band") == "20m", "rows in band order");
    check(act[2].toMap().value("total") == 3 && act[2].toMap().value("EU") == 1 && act[2].toMap().value("AS") == 0,
          "row exposes total and every continent");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
