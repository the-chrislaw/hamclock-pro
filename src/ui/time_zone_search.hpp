#pragma once
// Searchable IANA time zone list for the Settings picker (issue #151).
#include <QObject>
#include <QString>
#include <QVariantList>

namespace hamclock {

class TimeZoneSearch : public QObject {
    Q_OBJECT
public:
    explicit TimeZoneSearch(QObject* parent = nullptr) : QObject(parent) {}

    // Zones whose id or city matches every word of `query` (case-insensitive,
    // '_' counts as a space), each as {id, label}; label is "City (UTC-7)".
    // An empty query lists the first `limit` zones. `at_utc_secs` < 0 means now.
    Q_INVOKABLE QVariantList search(const QString& query, int limit = 60, qint64 at_utc_secs = -1) const;
    // Label for one zone id, empty when the id is unknown.
    Q_INVOKABLE QString labelFor(const QString& id, qint64 at_utc_secs = -1) const;
    // The system zone id when it is a known IANA id, else empty.
    Q_INVOKABLE QString systemZone() const;
};

} // namespace hamclock
