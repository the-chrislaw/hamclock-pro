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
}

} // namespace hamclock
