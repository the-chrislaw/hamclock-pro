#pragma once
// Parsers for NOAA SWPC JSON products (public US-government data, no key).
// Pure functions over the raw body: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <optional>
#include <vector>

namespace hamclock::swpc {

struct Reading {
    double value = 0;
    QDateTime time;  // UTC observation time from the product
};

// products/noaa-planetary-k-index.json: array of {time_tag, Kp, ...}.
// Latest row with a numeric Kp in [0, 9].
std::optional<Reading> latest_kp(const QByteArray& body);
// products/summary/10cm-flux.json: [{flux, time_tag}]. Solar flux index (sfu).
std::optional<Reading> latest_solar_flux(const QByteArray& body);
// json/goes/primary/xrays-*.json: rows per energy band; uses the 0.1-0.8 nm
// band (the one flare classes are defined on). Value is W/m^2.
std::optional<Reading> latest_xray_flux(const QByteArray& body);

// 0.1-0.8 nm series from xrays-1-day.json, oldest first, reduced to one point per
// `bucket_minutes` (the bucket maximum, so short flares survive the reduction).
std::vector<Reading> xray_series(const QByteArray& body, int bucket_minutes);
// Position of a flux on the plot's log axis: 0 at A1 (1e-8) to 1 at X10 (1e-3), clamped.
double xray_plot_fraction(double flux_w_m2);

// GOES flare class for a 0.1-0.8 nm flux, e.g. 3.2e-6 -> "C3.2", 1.2e-3 -> "X12".
QString xray_class(double flux_w_m2);
// Short geomagnetic description from Kp, using the NOAA G-scale (Kp 5 = G1 ... 9 = G5).
QString kp_description(double kp);

} // namespace hamclock::swpc
