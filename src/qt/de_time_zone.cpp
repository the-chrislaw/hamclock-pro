#include "de_time_zone.hpp"
#include <QByteArray>
#include <QDateTime>
#include <QTimeZone>

namespace hamclock {
bool is_valid_de_zone_id(const std::string& iana_id) {
    if (iana_id.empty()) return false;
    return QTimeZone(QByteArray::fromStdString(iana_id)).isValid();
}

std::optional<UtcOffsetSeconds> resolve_de_zone_offset(const std::string& iana_id, UtcSeconds utc) {
    if (iana_id.empty()) return std::nullopt;
    const QTimeZone zone(QByteArray::fromStdString(iana_id));
    if (!zone.isValid()) return std::nullopt;
    const auto instant = QDateTime::fromSecsSinceEpoch(utc.value, QTimeZone::utc());
    if (!instant.isValid()) return std::nullopt;
    return UtcOffsetSeconds{zone.offsetFromUtc(instant)};
}
} // namespace hamclock
