#pragma once
// Offline helpers for the Settings station form (backlog A0b): coordinates as
// decimal or degrees:minutes:seconds, Maidenhead grid entry, and parsing of
// OpenStreetMap Nominatim search results. No network access here.
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace hamclock {

class LocationTools : public QObject {
    Q_OBJECT
public:
    explicit LocationTools(QObject* parent = nullptr) : QObject(parent) {}

    // "40.5", "-74", "40:44:55N", "40°44'55\" S", "N 40.7". Hemisphere letters
    // must match the axis (N/S for latitude, E/W for longitude). Returns NaN
    // when the text is not a coordinate or is out of range.
    Q_INVOKABLE double parseCoordinate(const QString& text, bool latitude) const;
    // Centre of a 2, 4 or 6 character grid square as {latitude, longitude};
    // empty map when invalid.
    Q_INVOKABLE QVariantMap fromGrid(const QString& grid) const;
    // Six character grid square ("FN31pr") for a position; empty when out of range.
    Q_INVOKABLE QString gridFor(double latitude, double longitude) const;
    // Nominatim jsonv2 search reply -> list of {name, latitude, longitude}.
    // Entries without usable coordinates are dropped.
    Q_INVOKABLE QVariantList parseSearchReply(const QByteArray& json) const;
};

} // namespace hamclock
