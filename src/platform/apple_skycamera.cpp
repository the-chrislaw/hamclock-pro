#include "apple_skycamera.hpp"

namespace hamclock {

SkyCamera::SkyCamera(SkyMarker* marker, FovSource source, QObject* parent)
    : QObject(parent), marker_(marker)
{
    if (source) long_fov_ = source(&sensor_);
    if (!(long_fov_ > 0)) long_fov_ = 0;
}

void SkyCamera::setPreviewSize(double width, double height)
{
    if (!(width > 0) || !(height > 0)) return;
    const PreviewFov next = previewFov(long_fov_, sensor_, QSizeF(width, height));
    const bool changed = next.hfov_deg != fov_.hfov_deg || next.vfov_deg != fov_.vfov_deg;
    fov_ = next;
    if (marker_) marker_->setPreview(width, height, fov_.hfov_deg, fov_.vfov_deg);
    if (changed) emit previewChanged();
}

} // namespace hamclock
