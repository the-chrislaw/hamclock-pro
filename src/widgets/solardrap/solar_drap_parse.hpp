#pragma once
// Parser for the NOAA SWPC D-Region Absorption Prediction table (text/drap_global_frequencies.txt,
// public data, no key): highest affected frequency (MHz) on a 4 x 2 degree grid. Pure functions.
#include <QByteArray>
#include <QDateTime>
#include <QImage>
#include <QString>
#include "../../map/styles.hpp"
#include <vector>

namespace hamclock::solardrap {

constexpr int kCols = 90;  // longitude -178..178, step 4
constexpr int kRows = 90;  // latitude 89..-89, step 2 (north first)

struct Grid {
    QDateTime valid_at;           // UTC, "Product Valid At"
    QString xray, proton;         // SWPC messages, e.g. "Normal X-ray Background"
    std::vector<float> mhz;       // kRows x kCols, row 0 = latitude 89
    bool valid = false;
};

// Invalid grid unless the body has a valid time and kRows complete rows of kCols numbers.
Grid parse_drap(const QByteArray& body);

// Highest frequency anywhere on the grid, MHz.
double peak(const Grid& grid);

ColorScale drap_scale();

// World image for the map style: 360 x 180, longitude -180..179 left to right, north up.
// Transparent below 1 MHz.
QImage to_image(const Grid& grid);

} // namespace hamclock::solardrap
