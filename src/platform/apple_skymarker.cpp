#include "apple_skymarker.hpp"

#include <QVariantMap>

namespace hamclock {

SkyMarker::SkyMarker(SkySensor* sensor, SkyFinder* finder, QObject* parent)
    : QObject(parent), finder_(finder) {
    connect(sensor, &SkySensor::attitudeChanged, this, &SkyMarker::onAttitude);
    connect(finder, &SkyFinder::targetChanged, this, &SkyMarker::recompute);
}

void SkyMarker::setPreview(double width, double height, double hfov_deg, double vfov_deg) {
    size_ = QSizeF(width, height);
    view_.hfov_deg = hfov_deg;
    view_.vfov_deg = vfov_deg;
    recompute();
}

void SkyMarker::onAttitude(double az, double el) {
    view_.az_deg = az;
    view_.el_deg = el;
    have_attitude_ = true;
    recompute();
}

void SkyMarker::setTrack(const std::vector<std::pair<double, double>>& azel) {
    track_ = azel;
    recompute();
}

void SkyMarker::recompute() {
    QVariantList pts;
    if (have_attitude_ && size_.width() > 0 && size_.height() > 0) {
        bool gap = false;
        for (const auto& s : track_) {
            const ScreenPoint sp = projectToScreen(view_, s.first, s.second, size_);
            if (!sp.in_front || !sp.on_screen) { gap = true; continue; }
            QVariantMap m;
            m["x"] = sp.pos.x();
            m["y"] = sp.pos.y();
            m["gap"] = gap && !pts.isEmpty();
            gap = false;
            pts.append(m);
        }
    }
    const bool tracks_same = pts == track_pts_;
    track_pts_ = pts;
    ScreenPoint next;
    if (have_attitude_ && finder_->haveTarget() && size_.width() > 0 && size_.height() > 0)
        next = projectToScreen(view_, finder_->targetAz(), finder_->targetEl(), size_);
    const bool same = next.in_front == pt_.in_front && next.on_screen == pt_.on_screen &&
                      next.pos == pt_.pos && next.edge_angle_deg == pt_.edge_angle_deg;
    pt_ = next;
    if (!same || !tracks_same) emit changed();
}

} // namespace hamclock
