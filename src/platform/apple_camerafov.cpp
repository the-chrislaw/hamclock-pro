#include "apple_camerafov.hpp"

#include <algorithm>
#include <cmath>

namespace hamclock {

PreviewFov previewFov(double sensor_long_fov_deg, const QSizeF &sensor_px, const QSizeF &preview)
{
    if (!(sensor_long_fov_deg > 1 && sensor_long_fov_deg < 170) || sensor_px.width() <= 0 || sensor_px.height() <= 0
        || preview.width() <= 0 || preview.height() <= 0)
        return {};
    const double kPi = 3.14159265358979323846;
    const double long_px = std::max(sensor_px.width(), sensor_px.height());
    const double short_px = std::min(sensor_px.width(), sensor_px.height());
    const bool portrait = preview.height() > preview.width();
    const double sw = portrait ? short_px : long_px;
    const double sh = portrait ? long_px : short_px;
    // Focal length in sensor pixels.
    const double f = (long_px / 2) / std::tan(sensor_long_fov_deg * kPi / 360);
    // Aspect fill: the larger scale wins, the other axis is cropped.
    const double s = std::max(preview.width() / sw, preview.height() / sh);
    const double half_w = preview.width() / s / 2;
    const double half_h = preview.height() / s / 2;
    return {2 * std::atan(half_w / f) * 180 / kPi, 2 * std::atan(half_h / f) * 180 / kPi};
}

} // namespace hamclock
