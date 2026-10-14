#include "time_zone_search.hpp"

#include <QDateTime>
#include <QStringList>
#include <QTimeZone>
#include <QVariantMap>

#include <algorithm>

namespace hamclock {
namespace {

QDateTime when(qint64 at_utc_secs) {
    return at_utc_secs < 0 ? QDateTime::currentDateTimeUtc() : QDateTime::fromSecsSinceEpoch(at_utc_secs, Qt::UTC);
}

QString city_of(const QString& id) { return id.mid(id.lastIndexOf('/') + 1).replace('_', ' '); }

QString offset_text(int seconds) {
    const QChar sign = seconds < 0 ? QChar(0x2212) : QChar('+');
    const int abs_minutes = std::abs(seconds) / 60;
    QString text = QStringLiteral("UTC") + sign + QString::number(abs_minutes / 60);
    if (abs_minutes % 60) text += QString(":%1").arg(abs_minutes % 60, 2, 10, QChar('0'));
    return text;
}

QString label(const QString& id, const QDateTime& at) {
    const QTimeZone tz(id.toUtf8());
    return city_of(id) + " (" + offset_text(tz.offsetFromUtc(at)) + ")";
}

} // namespace

QVariantList TimeZoneSearch::search(const QString& query, int limit, qint64 at_utc_secs) const {
    const auto words = query.toLower().replace('_', ' ').split(' ', Qt::SkipEmptyParts);
    auto ids = QTimeZone::availableTimeZoneIds();
    std::sort(ids.begin(), ids.end());
    const auto at = when(at_utc_secs);
    QVariantList out;
    for (const auto& raw : ids) {
        const QString id = QString::fromUtf8(raw);
        if (!id.contains('/')) continue;  // skip legacy aliases such as "EST5EDT"
        const QString haystack = id.toLower().replace('_', ' ');
        if (!std::all_of(words.begin(), words.end(), [&](const QString& w) { return haystack.contains(w); })) continue;
        out.push_back(QVariantMap{{"id", id}, {"label", label(id, at)}, {"region", id.left(id.indexOf('/'))}});
        if (out.size() >= limit) break;
    }
    return out;
}

QString TimeZoneSearch::labelFor(const QString& id, qint64 at_utc_secs) const {
    if (!QTimeZone::isTimeZoneIdAvailable(id.toUtf8())) return {};
    return label(id, when(at_utc_secs));
}

QString TimeZoneSearch::systemZone() const {
    const QString id = QString::fromUtf8(QTimeZone::systemTimeZoneId());
    return QTimeZone::isTimeZoneIdAvailable(id.toUtf8()) ? id : QString();
}

} // namespace hamclock
