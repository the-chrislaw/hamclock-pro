#pragma once
// Parser for the USGS earthquake GeoJSON summary feeds
// (earthquake.usgs.gov/earthquakes/feed/v1.0/summary/*.geojson). US-government
// public-domain data, no key. Pure functions: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::quakes {

struct Quake {
    double magnitude = 0;
    QString place;       // "126 km S of Kokopo, Papua New Guinea"
    QDateTime time;      // origin time, UTC
    double latitude = 0, longitude = 0, depth_km = 0;
    bool tsunami = false;  // USGS tsunami-flag: a tsunami message may exist
    QString alert;         // PAGER level (green/yellow/orange/red) or empty
    QString url;
};

struct Feed {
    bool ok = false;
    QDateTime generated;  // when USGS built the feed, UTC
    std::vector<Quake> quakes;  // newest first
};

// Rows with no numeric magnitude, a bad origin time or bad coordinates are
// skipped. A body that is not a GeoJSON FeatureCollection yields ok == false.
Feed parse(const QByteArray& body);

// Quake with the greatest magnitude (earliest in the list on ties); nullptr if none.
const Quake* strongest(const Feed& feed);

// Plain-language size class: Great / Major / Strong / Moderate / Light / Minor.
QString magnitude_class(double magnitude);

} // namespace hamclock::quakes
