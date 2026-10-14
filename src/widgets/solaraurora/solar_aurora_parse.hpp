#pragma once
// Parser for the NOAA SWPC OVATION aurora nowcast (json/ovation_aurora_latest.json, public data,
// no key): a 1 degree grid of [longitude 0..359, latitude -90..90, probability %]. Pure functions.
#include <QByteArray>
#include <QDateTime>
#include <QImage>
#include "../../map/styles.hpp"
#include <vector>

namespace hamclock::solaraurora {

struct Grid {
    QDateTime observed, forecast;  // UTC
    std::vector<unsigned char> cells;  // 360 x 181, index (lat + 90) * 360 + lon; missing cells are 0
    bool valid = false;
    int at(int lat, int lon) const { return cells[static_cast<size_t>((lat + 90) * 360 + ((lon % 360) + 360) % 360)]; }
};

// Invalid grid when the body is not the expected object or has no usable cells.
Grid parse_ovation(const QByteArray& body);

// Highest probability in the northern (lat >= 0) or southern hemisphere.
int peak(const Grid& grid, bool north);

// Colour scale shared by the image and the map legend (probability %).
ColorScale aurora_scale();

// World image for the map style: 360 x 180, longitude -180..179 left to right, north up.
// Transparent below 3 %, opaque from 30 %; green through yellow to red.
QImage to_image(const Grid& grid);

} // namespace hamclock::solaraurora
