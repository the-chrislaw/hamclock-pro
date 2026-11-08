#include "apple_skytarget.hpp"

namespace hamclock {

bool finderTargetFromDetail(const QVariantMap& detail, double& az, double& el) {
    if (!detail.contains("az") || !detail.contains("el")) return false;
    if (!detail.value("above").toBool()) return false;
    bool okAz = false, okEl = false;
    const double a = detail.value("az").toDouble(&okAz);
    const double e = detail.value("el").toDouble(&okEl);
    if (!okAz || !okEl || a < 0 || a > 360 || e < 0 || e > 90) return false;
    az = a;
    el = e;
    return true;
}

std::vector<std::pair<double, double>> trackFromSkyTrack(const QVariantList& list) {
    std::vector<std::pair<double, double>> out;
    for (const QVariant& v : list) {
        const QVariantMap m = v.toMap();
        bool okAz = false, okEl = false;
        const double az = m.value("az").toDouble(&okAz);
        const double el = m.value("el").toDouble(&okEl);
        if (okAz && okEl) out.emplace_back(az, el);
    }
    return out;
}

SkyTrackBridge::SkyTrackBridge(QObject* source, SkyMarker* marker, QObject* parent)
    : QObject(parent), source_(source), marker_(marker) {
    if (source_) connect(source_, SIGNAL(changed()), this, SLOT(sync()));
    sync();
}

void SkyTrackBridge::sync() {
    if (!source_ || !marker_) return;
    const QVariantList cur = source_->property("skyTrack").toList();
    if (cur == last_) return; // the model signals `changed` for every refresh; only redraw on a new track
    last_ = cur;
    marker_->setTrack(trackFromSkyTrack(cur));
}

SkyTargetBridge::SkyTargetBridge(QObject* source, SkyFinder* finder, QObject* parent)
    : QObject(parent), source_(source), finder_(finder) {
    if (source_) connect(source_, SIGNAL(detailChanged()), this, SLOT(sync()));
    sync();
}

void SkyTargetBridge::sync() {
    if (!source_ || !finder_) return;
    double az = 0, el = 0;
    have_ = finderTargetFromDetail(source_->property("detail").toMap(), az, el);
    if (have_) finder_->setTarget(az, el);
    else finder_->clearTarget();
}

} // namespace hamclock
