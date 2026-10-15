#include "muf_map.hpp"
#include "path_model.hpp"

namespace hamclock::bands {

ColorScale muf_scale() {
    return {"MUF", "MHz",
            {{5, QColor(60, 20, 120)}, {10, QColor(30, 90, 200)}, {15, QColor(30, 170, 170)},
             {20, QColor(90, 200, 70)}, {28, QColor(240, 210, 40)}, {36, QColor(230, 70, 40)}}};
}

QImage muf_image(double sfi, UtcSeconds utc) {
    QImage img(360, 180, QImage::Format_ARGB32);
    const ColorScale scale = muf_scale();
    for (int row = 0; row < 180; ++row) {
        const double lat = 89.5 - row;
        for (int x = 0; x < 360; ++x)
            img.setPixelColor(x, row, color_at(scale, muf_3000_mhz(sfi, lat, x - 179.5, utc)));
    }
    return img;
}

} // namespace hamclock::bands
