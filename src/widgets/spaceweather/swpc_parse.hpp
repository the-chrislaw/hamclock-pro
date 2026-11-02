#pragma once
// Parsers for NOAA SWPC JSON products (public US-government data, no key).
// Pure functions over the raw body: no network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <optional>
#include <vector>

namespace hamclock::swpc {

struct Reading {
    double value = 0;
    QDateTime time;  // UTC observation time from the product
};

// How old a shown value may be, measured from the product's own time tag: a feed can
// answer on time with rows that have stopped moving.
// Kp: the tag is the START of a 3 h interval and the row appears after its end, so a
// healthy feed reaches 6 h just before the next row. The limit allows one missed row.
inline constexpr qint64 kKpMaxAgeSecs = 9 * 3600;
// SFI: one reading a day, tagged 20:00 UTC. 72 h covers a Friday reading still showing
// on Monday evening, and 12 h more lets Monday's arrive late.
inline constexpr qint64 kSolarFluxMaxAgeSecs = 84 * 3600;
// X-ray: a new row every minute. Matches the X-ray product's fetch limit.
inline constexpr qint64 kXrayMaxAgeSecs = 30 * 60;

// False for an unknown time and for a tag ahead of the clock.
bool is_old(const QDateTime& time, const QDateTime& now_utc, qint64 max_age_secs);
// Footer note for values that are old or absent, short enough for the 800x480 footer
// with every index named: "SWPC · old Kp, SFI, no X-ray".
QString stale_note(const QStringList& old_labels, const QStringList& missing_labels = {});

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
