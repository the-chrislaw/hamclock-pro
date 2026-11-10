#include "apple_skycamera.hpp"

#include <QPointer>

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

QString SkyCamera::state() const
{
    if (!available()) return QStringLiteral("unavailable");
    if (!enabled_) return QStringLiteral("off");
    if (requesting_) return QStringLiteral("requesting");
    return granted_ ? QStringLiteral("on") : QStringLiteral("denied");
}

void SkyCamera::setEnabled(bool on)
{
    if (on == enabled_) return;
    if (!available()) return; // arrow mode only
    enabled_ = on;
    ++epoch_;
    if (!on) { requesting_ = false; emit stateChanged(); return; }
    if (granted_) { emit stateChanged(); return; }
    if (!request_) { granted_ = false; emit stateChanged(); return; }
    requesting_ = true;
    emit stateChanged();
    const quint64 epoch = epoch_;
    QPointer<SkyCamera> self(this);
    request_([self, epoch](bool ok) {
        if (!self || self->epoch_ != epoch) return; // switched off or on again meanwhile
        self->requesting_ = false;
        self->granted_ = ok;
        emit self->stateChanged();
    });
}

} // namespace hamclock
