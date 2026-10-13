#pragma once
// Parsers for NOAA SWPC long-baseline space weather products (public US-government
// data, no key): 3-day Kp, daily 10.7 cm flux history, monthly sunspot number.
// Pure functions over the raw body: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::solartrend {

struct KpBar {
    QDateTime time;  // UTC start of the 3-hour interval
    double kp = 0;
    bool predicted = false;  // forecast rather than observed/estimated
};

struct FluxDay {
    QDate date;      // UTC
    double flux = 0; // sfu
};

struct Sunspots {
    QString month;   // "YYYY-MM"
    double number = 0;
};

// products/noaa-planetary-k-index-forecast.json: [{time_tag, kp, observed}].
// Returns the bars from 36 h before the newest non-predicted bar to the end of
// the forecast, oldest first. Rows with Kp outside [0, 9] are skipped.
std::vector<KpBar> kp_bars(const QByteArray& body);

// json/f107_cm_flux.json: three readings a day (Morning/Noon/Afternoon). The
// local-noon (Noon) reading is the official daily value; days without one are
// skipped. Returns at most `days` newest days, oldest first.
std::vector<FluxDay> flux_daily(const QByteArray& body, int days = 30);

// json/solar-cycle/observed-solar-cycle-indices.json: newest month with a
// non-negative NOAA sunspot number (-1 marks a missing value).
bool latest_sunspots(const QByteArray& body, Sunspots* out);

// Kp bar colour class: 0 quiet, 1 active (Kp 4), 2 storm (Kp >= 5, NOAA G1+).
int kp_level(double kp);

} // namespace hamclock::solartrend
