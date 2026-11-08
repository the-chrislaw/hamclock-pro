#pragma once
// Feeds the sky finder (#573) its target from the satellites card's selected-
// satellite detail (az/el at the station, kept fresh by that model). Platform-free
// so it is tested on Linux; main.cpp creates it on iOS only.
#include "apple_skyfinder.hpp"
#include "apple_skymarker.hpp"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <utility>
#include <vector>

namespace hamclock {

// True and fills az/el only when the detail has a satellite above the horizon.
bool finderTargetFromDetail(const QVariantMap& detail, double& az, double& el);

// az/el pairs from a satellites-model `skyTrack` list ([{az, el}]); malformed entries are skipped.
std::vector<std::pair<double, double>> trackFromSkyTrack(const QVariantList& list);

// Feeds SkyMarker's pass-track polyline from the satellites model's `skyTrack`
// (the soonest pass's az/el samples). `source` needs a `skyTrack` QVariantList
// property and a `changed()` signal.
class SkyTrackBridge : public QObject {
    Q_OBJECT
public:
    SkyTrackBridge(QObject* source, SkyMarker* marker, QObject* parent = nullptr);

public slots:
    void sync();

private:
    QObject* source_;
    SkyMarker* marker_;
    QVariantList last_;
};

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
