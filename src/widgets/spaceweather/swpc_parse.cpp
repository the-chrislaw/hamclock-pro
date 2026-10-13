#include "swpc_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

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
