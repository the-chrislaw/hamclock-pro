#include "swpc_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>
#include <map>

namespace hamclock::swpc {
namespace {
constexpr char kFlareBand[] = "0.1-0.8nm";

QDateTime parse_time(const QJsonValue& value) {
    auto text = value.toString().trimmed();
    if (text.isEmpty()) return {};
    if (!text.endsWith('Z')) text += 'Z'; // SWPC tags are UTC with or without the suffix
    const auto time = QDateTime::fromString(text, Qt::ISODate);
    return time.isValid() ? time.toUTC() : QDateTime{};
}

QJsonArray rows(const QByteArray& body) {
    QJsonParseError error{};
    const auto document = QJsonDocument::fromJson(body, &error);
    return error.error == QJsonParseError::NoError && document.isArray() ? document.array() : QJsonArray{};
}

// Latest (by time) row accepted by `read`, which returns the numeric value or nullopt.
template <class Read>
std::optional<Reading> latest(const QByteArray& body, Read read) {
    std::optional<Reading> best;
    for (const auto& item : rows(body)) {
        if (!item.isObject()) continue;
        const auto row = item.toObject();
        const auto time = parse_time(row.value("time_tag"));
        const auto value = read(row);
        if (!time.isValid() || !value || !std::isfinite(*value)) continue;
        if (!best || time >= best->time) best = Reading{*value, time};
    }
    return best;
}
} // namespace

std::optional<Reading> latest_kp(const QByteArray& body) {
    return latest(body, [](const QJsonObject& row) -> std::optional<double> {
        const auto kp = row.value("Kp");
        if (!kp.isDouble() || kp.toDouble() < 0 || kp.toDouble() > 9) return std::nullopt;
        return kp.toDouble();
    });
}

std::optional<Reading> latest_solar_flux(const QByteArray& body) {
    return latest(body, [](const QJsonObject& row) -> std::optional<double> {
        const auto flux = row.value("flux");
        if (!flux.isDouble() || flux.toDouble() <= 0 || flux.toDouble() > 1000) return std::nullopt;
        return flux.toDouble();
    });
}

std::optional<Reading> latest_xray_flux(const QByteArray& body) {
    return latest(body, [](const QJsonObject& row) -> std::optional<double> {
        const auto flux = row.value("flux");
        if (row.value("energy").toString() != kFlareBand || !flux.isDouble() || flux.toDouble() <= 0)
            return std::nullopt;
        return flux.toDouble();
    });
}

std::vector<Reading> xray_series(const QByteArray& body, int bucket_minutes) {
    std::vector<Reading> out;
    if (bucket_minutes < 1) return out;
    const qint64 bucket_ms = qint64(bucket_minutes) * 60 * 1000;
    std::map<qint64, Reading> buckets;
    for (const auto& item : rows(body)) {
        if (!item.isObject()) continue;
        const auto row = item.toObject();
        const auto flux = row.value("flux");
        const auto time = parse_time(row.value("time_tag"));
        if (row.value("energy").toString() != kFlareBand || !flux.isDouble() || !time.isValid() ||
            !std::isfinite(flux.toDouble()) || flux.toDouble() <= 0) continue;
        const qint64 key = time.toMSecsSinceEpoch() / bucket_ms;
        auto it = buckets.find(key);
        if (it == buckets.end()) buckets.emplace(key, Reading{flux.toDouble(), time});
        else if (flux.toDouble() > it->second.value) it->second.value = flux.toDouble();
    }
    for (const auto& [key, reading] : buckets) out.push_back({reading.value, QDateTime::fromMSecsSinceEpoch(key * bucket_ms, Qt::UTC)});
    return out;
}

double xray_plot_fraction(double flux) {
    if (!std::isfinite(flux) || flux <= 1e-8) return 0;
    return std::min(1.0, (std::log10(flux) + 8) / 5);
}

QString xray_class(double flux) {
    if (!std::isfinite(flux) || flux <= 0) return {};
    struct Band { char letter; double base; };
    static constexpr Band bands[] = {{'X', 1e-4}, {'M', 1e-5}, {'C', 1e-6}, {'B', 1e-7}, {'A', 1e-8}};
    for (const auto& band : bands) {
        if (flux >= band.base || band.letter == 'A') {
            // Tolerance absorbs binary rounding (1.2e-3 / 1e-4 = 11.999...); class
            // digits are truncated, not rounded, per the GOES convention.
            constexpr double kEpsilon = 1e-9;
            const double scaled = flux / band.base;
            if (band.letter == 'X' && scaled >= 10) return QString("X%1").arg(std::floor(scaled + kEpsilon), 0, 'f', 0);
            return QString("%1%2").arg(band.letter).arg(std::floor(scaled * 10 + kEpsilon) / 10, 0, 'f', 1);
        }
    }
    return {};
}

QString kp_description(double kp) {
    if (!std::isfinite(kp)) return {};
    if (kp >= 9) return "Extreme storm (G5)";
    if (kp >= 8) return "Severe storm (G4)";
    if (kp >= 7) return "Strong storm (G3)";
    if (kp >= 6) return "Moderate storm (G2)";
    if (kp >= 5) return "Minor storm (G1)";
    if (kp >= 4) return "Active";
    return "Quiet";
}

} // namespace hamclock::swpc
