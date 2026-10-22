#pragma once
// Parser for SondeHub radiosonde telemetry
// (api.v2.sondehub.org/sondes/telemetry?duration=1h): {serial: {time: frame}}.
// Pure functions: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::balloons {

struct Sonde {
    QString serial;
    QString type;       // "RS41", "DFM", ...
    double lat = 0, lon = 0, alt_m = 0;
    bool has_vel_v = false;
    double vel_v = 0;   // m/s, + climbing
    double freq_mhz = 0;  // 0 when unknown
    QDateTime time;     // time of the newest frame, UTC
    bool has_prev = false;  // an earlier frame (> 0 s older) was also heard
    double prev_lat = 0, prev_lon = 0;
    QDateTime prev_time;
};

struct Feed {
    bool ok = false;
    std::vector<Sonde> sondes;  // newest frame per serial
};

// Keeps the newest frame of each serial; frames without a usable position or
// time are skipped. A body that is not a JSON object yields ok == false.
Feed parse(const QByteArray& body);

// "climbing", "descending" or "" when the vertical speed is unknown or the
// sonde is level (|v| < 1 m/s).
QString phase(const Sonde& s);

// Rough seconds until a descending sonde reaches the ground, or -1 when it is
// not clearly descending (< 2 m/s down) or the vertical speed is unknown.
// Assumes the fall speed scales with 1/sqrt(air density) in an exponential
// atmosphere (scale height 7.5 km), anchored at the current speed. Ignores
// the terrain height and wind, so it is only an estimate.
int landing_eta_s(const Sonde& s);

// Time still to go given the age of the report: landing_eta_s minus age_s,
// 0 when that has already elapsed (the sonde has probably landed), -1 unknown.
int landing_eta_remaining_s(const Sonde& s, qint64 age_s);

// Where a descending sonde may come down: its newest position moved on by the
// drift between its last two frames for the time still to go. Ignores that the
// wind usually weakens towards the ground, so it tends to overshoot. Returns
// false when it is not clearly descending, the drift is unknown or the report
// has already run out (probably landed).
bool landing_point(const Sonde& s, qint64 age_s, double& lat, double& lon);

// Sondes heard within `max_age_s` of `now`, nearest to (lat, lon) first, at
// most `limit`. `distance_km` is parallel to the result.
struct Near { Sonde sonde; double distance_km; double bearing_deg; };
std::vector<Near> nearest(const Feed& feed, double lat, double lon, const QDateTime& now, int max_age_s, int limit);

} // namespace hamclock::balloons
