#include "beacons_model.hpp"
#include "../../map/overlay.hpp"
#include <QVariantMap>

namespace hamclock {

QVariantList beacon_rows(UtcSeconds now) {
    QVariantList rows;
    for (int band = 0; band < kNcdxfBandCount; ++band) {
        const BeaconSlot slot = ncdxf_current(now, band);
        const NcdxfBeacon& b = ncdxf_beacon(slot.beacon_index);
        rows.append(QVariantMap{
            {"band", QString::number(ncdxf_band_khz(band) / 1000.0, 'f', 3)},
            {"call", QString::fromLatin1(b.call)},
            {"location", QString::fromLatin1(b.location)},
            {"remaining", slot.seconds_remaining}});
    }
    return rows;
}

OverlayLayer beacon_overlay_layer(UtcSeconds now) {
    OverlayLayer layer;
    layer.id = "beacons";
    layer.labels = OverlayLabelMode::Call;
    layer.min_label_distance_px = 30;
    for (int band = 0; band < kNcdxfBandCount; ++band) {
        const int index = ncdxf_current(now, band).beacon_index;
        OverlayMarker m;
        m.where = ncdxf_beacon_location(index);
        m.call = QString::fromLatin1(ncdxf_beacon(index).call);
        m.detail = QString("%1 MHz, %2").arg(ncdxf_band_khz(band) / 1000.0, 0, 'f', 3).arg(QString::fromLatin1(ncdxf_beacon(index).location));
        m.color = QColor("#f2a900");
        m.shape = OverlayShape::Diamond;
        m.size_px = 5;
        layer.markers.push_back(m);
    }
    return layer;
}

BeaconsModel::BeaconsModel(const WidgetContext&, QObject* parent)
    : WidgetModel("beacons", "Beacons", parent) {
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, &BeaconsModel::recompute);
}

void BeaconsModel::start() { recompute(); timer_.start(); }

void BeaconsModel::recompute() {
    rows_ = beacon_rows(UtcSeconds{now().toSecsSinceEpoch()});
    MapOverlays::instance().set_layer(beacon_overlay_layer(UtcSeconds{now().toSecsSinceEpoch()}));
    setState("live", "NCDXF/IARU schedule from the system clock", now());
    emit changed();
}

void register_widget_beacons(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new BeaconsModel(context);
    registry.add({"beacons", "Beacons", QUrl("qrc:/widgets/beacons/BeaconsCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &BeaconsModel::start);
}

} // namespace hamclock
