#include "solar_trend_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <algorithm>
#include <cmath>
#include <map>

namespace hamclock::solartrend {
namespace {
QJsonArray rows(const QByteArray& body) {
    const auto doc = QJsonDocument::fromJson(body);
    return doc.isArray() ? doc.array() : QJsonArray{};
}

QDateTime parse_time(const QString& text) {
    auto time = QDateTime::fromString(text.trimmed().remove('Z'), Qt::ISODate);  // SWPC tags are UTC
    if (!time.isValid()) return {};
    time.setTimeZone(QTimeZone::utc());
    return time;
}
} // namespace

std::vector<KpBar> kp_bars(const QByteArray& body) {
    std::vector<KpBar> all;
    for (const auto& item : rows(body)) {
        const auto row = item.toObject();
        const auto kp = row.value("kp");
        const auto time = parse_time(row.value("time_tag").toString());
        if (!time.isValid() || !kp.isDouble() || !(kp.toDouble() >= 0 && kp.toDouble() <= 9)) continue;
        all.push_back({time, kp.toDouble(), row.value("observed").toString() == "predicted"});
    }
    std::stable_sort(all.begin(), all.end(), [](const KpBar& a, const KpBar& b) { return a.time < b.time; });
    all.erase(std::unique(all.begin(), all.end(), [](const KpBar& a, const KpBar& b) { return a.time == b.time; }),
              all.end());
    QDateTime newest_real;
    for (const auto& bar : all)
        if (!bar.predicted) newest_real = bar.time;
    if (!newest_real.isValid()) return {};  // a forecast alone is not an observation
    const auto start = newest_real.addSecs(-36 * 3600);
    all.erase(all.begin(), std::find_if(all.begin(), all.end(), [&](const KpBar& b) { return b.time >= start; }));
    return all;
}

std::vector<FluxDay> flux_daily(const QByteArray& body, int days) {
    std::map<QDate, double> by_day;
    for (const auto& item : rows(body)) {
        const auto row = item.toObject();
        const auto flux = row.value("flux");
        const auto time = parse_time(row.value("time_tag").toString());
        if (row.value("reporting_schedule").toString() != "Noon" || !time.isValid() || !flux.isDouble()) continue;
        if (!(flux.toDouble() > 0 && flux.toDouble() <= 1000)) continue;
        by_day[time.date()] = flux.toDouble();
    }
    std::vector<FluxDay> out;
    for (const auto& [date, flux] : by_day) out.push_back({date, flux});
    if (days > 0 && out.size() > static_cast<size_t>(days)) out.erase(out.begin(), out.end() - days);
    return out;
}

bool latest_sunspots(const QByteArray& body, Sunspots* out) {
    QString best_month;
    double best = 0;
    for (const auto& item : rows(body)) {
        const auto row = item.toObject();
        const auto month = row.value("time-tag").toString();
        const auto ssn = row.value("observed_swpc_ssn");
        if (month.size() != 7 || !ssn.isDouble() || !(ssn.toDouble() >= 0)) continue;
        if (month > best_month) { best_month = month; best = ssn.toDouble(); }  // ISO months sort lexically
    }
    if (best_month.isEmpty()) return false;
    *out = {best_month, best};
    return true;
}

int kp_level(double kp) { return kp >= 5 ? 2 : kp >= 4 ? 1 : 0; }

} // namespace hamclock::solartrend
