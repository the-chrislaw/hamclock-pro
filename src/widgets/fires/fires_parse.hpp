#pragma once
// Parser for NASA FIRMS active-fire CSV (VIIRS NOAA-20 near-real-time, regional
// 24 h file). NASA open data, no key. Pure functions: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <vector>

namespace hamclock::fires {

struct Detection {
    double latitude = 0, longitude = 0;
    QDateTime acquired;     // satellite overpass, UTC
    double frp_mw = 0;      // fire radiative power
    enum Confidence { Low, Nominal, High } confidence = Nominal;
};

struct Feed {
    bool ok = false;
    std::vector<Detection> detections;  // strongest (FRP) first
};

// A body without the FIRMS header columns yields ok == false; a header-only
// file is valid (no detections). Malformed rows are skipped.
Feed parse(const QByteArray& body);

} // namespace hamclock::fires
