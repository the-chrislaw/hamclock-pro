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
    // Nearest known city's IANA zone for a coordinate (offline table of ~110 cities;
    // a hint only: borders are not modelled). Empty for invalid coordinates or when
    // the nearest city is more than 1500 km away (open ocean, poles).
    Q_INVOKABLE QString suggestZone(double latitude, double longitude) const;
    // The system zone id when it is a known IANA id, else empty.
    Q_INVOKABLE QString systemZone() const;
    // The zone to use for the station: the saved one when set, else the zone suggested
    // from the station's coordinates, and only without a usable location the system zone.
    Q_INVOKABLE QString stationZone(const QString& saved, double latitude, double longitude) const;
};

} // namespace hamclock
