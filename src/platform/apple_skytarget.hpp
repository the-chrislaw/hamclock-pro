#pragma once
// Feeds the sky finder (#573) its target from the satellites card's selected-
// satellite detail (az/el at the station, kept fresh by that model). Platform-free
// so it is tested on Linux; main.cpp creates it on iOS only.
#include "apple_skyfinder.hpp"

#include <QObject>
#include <QVariantMap>

namespace hamclock {

// True and fills az/el only when the detail has a satellite above the horizon.
bool finderTargetFromDetail(const QVariantMap& detail, double& az, double& el);

class SkyTargetBridge : public QObject {
    Q_OBJECT
public:
    // `source` needs a `detail` QVariantMap property and a `detailChanged()` signal.
    SkyTargetBridge(QObject* source, SkyFinder* finder, QObject* parent = nullptr);
    bool hasTarget() const { return have_; }

public slots:
    void sync();

private:
    QObject* source_;
    SkyFinder* finder_;
    bool have_ = false;
};

} // namespace hamclock
