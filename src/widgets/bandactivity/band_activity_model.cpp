#include "band_activity_model.hpp"
#include "band_activity.hpp"
#include <QMetaMethod>
#include <QTimer>

namespace hamclock {

BandActivityModel::BandActivityModel(const WidgetContext&, QObject* parent)
    : WidgetModel("bandactivity", "Band activity", parent) {
    setState("unconfigured", "Needs the DX cluster pane");
}

QStringList BandActivityModel::continents() const { return bandactivity::continents(); }

void BandActivityModel::attach(QObject* source) {
    source_ = source;
    if (!source) return;
    // The cluster model signals on every spot change and on every state change.
    for (const char* sig : {"countChanged()", "stateChanged()"}) {
        const int idx = source->metaObject()->indexOfSignal(sig);
        if (idx >= 0) {
            const auto m = source->metaObject()->method(idx);
            const int slot = metaObject()->indexOfSlot("refresh()");
            if (slot >= 0) connect(source, m, this, metaObject()->method(slot));
        }
    }
    refresh();
}

void BandActivityModel::refresh() {
    if (!source_) return;
    rows_ = source_->property("activity").toList();
    max_cell_ = 0;
    int best_total = 0;
    QString best;
    for (const auto& v : rows_) {
        const QVariantMap m = v.toMap();
        for (const auto& c : bandactivity::continents()) max_cell_ = std::max(max_cell_, m.value(c).toInt());
        if (m.value("total").toInt() > best_total) { best_total = m.value("total").toInt(); best = m.value("band").toString(); }
    }
    busiest_ = best.isEmpty() ? QString() : best + " · " + QString::number(best_total) + " spots";

    const QString src_state = source_->property("state").toString();
    const QDateTime updated = source_->property("updatedAt").toDateTime();
    if (src_state == "unconfigured") setState("unconfigured", "Needs a DX cluster host and callsign");
    else if (src_state == "live" && !rows_.isEmpty()) setState("live", "Spots from the DX cluster", updated);
    else if (src_state == "live") setState("loading", "Waiting for spots", updated);
    else if (src_state == "stale" || src_state == "offline" || src_state == "error")
        setState(src_state == "stale" ? "stale" : "offline", "DX cluster link is " + src_state, updated);
    else setState("loading", "Waiting for the DX cluster");
    emit activityChanged();
}

void register_widget_bandactivity(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new BandActivityModel(context);
    registry.add({"bandactivity", "Band activity", QUrl("qrc:/widgets/bandactivity/BandActivityCard.qml"), "side", 45}, model);
    QTimer::singleShot(0, model, [model, &registry] { model->attach(registry.model("dxcluster")); });
}

} // namespace hamclock
