#pragma once
// Rough HF band outlook from solar flux (SFI), Kp and the A index. This is a
// hand-written heuristic in the spirit of the HamQSL/N0NBH summary, NOT a
// VOACAP prediction and not path-specific. Model:
//   * Day MUF (MHz) = 15 + 0.15 * (SFI - 70); night MUF = 0.65 * day MUF
//     (floor 6 MHz). SFI is clamped to [60, 300].
//   * A band is Good if f <= 0.85 * MUF, Fair if f <= MUF, else Poor.
//   * D-layer absorption by day: 80m is Poor, 40m is at best Fair.
//   * Geomagnetic storm: Kp >= 5 or A >= 30 lowers every band one step;
//     Kp >= 7 or A >= 80 lowers it two steps.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <optional>
#include <vector>

namespace hamclock::bands {

enum class Condition { Poor, Fair, Good };
struct BandOutlook {
    QString band;   // "20m"
    double mhz = 0; // representative frequency
    Condition day = Condition::Poor;
    Condition night = Condition::Poor;
};
struct IndexReading {
    double value = 0;
    QDateTime time;
};

std::vector<BandOutlook> hf_outlook(double sfi, double kp, double a_index);
QString condition_name(Condition condition);
// products/noaa-planetary-k-index.json: latest row with a numeric a_running in [0, 400].
std::optional<IndexReading> latest_a_index(const QByteArray& body);
// "day" when the Sun is above the horizon at the site, else "night".
QString period_at(const QDateTime& utc, double latitude_deg, double longitude_deg);

} // namespace hamclock::bands
