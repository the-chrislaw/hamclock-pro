#include "eclipse_model.hpp"
#include <QDateTime>
#include <QTimeZone>

namespace hamclock {

EclipseSummary summarize_eclipse(UtcSeconds now) {
    EclipseSummary out;
    const auto e = next_solar_eclipse(now);
    if (!e) return out;
    out.kind = QString("%1 solar eclipse").arg(solar_eclipse_type_name(e->type));
    out.when = QDateTime::fromSecsSinceEpoch(e->greatest.value, QTimeZone::utc()).toString("d MMM yyyy HH:mm") + "Z";
    const std::int64_t minutes = (e->greatest.value - now.value) / 60;
    const std::int64_t days = minutes / 1440;
    out.countdown = days >= 1 ? QString("in %1 d").arg(days)
                              : QString("in %1 h %2 min").arg(minutes / 60).arg(minutes % 60, 2, 10, QChar('0'));
    out.detail = e->type == SolarEclipseType::Partial
        ? QString("Magnitude %1, greatest eclipse").arg(e->magnitude, 0, 'f', 2)
        : QString("Central, gamma %1").arg(e->gamma, 0, 'f', 2);
    return out;
}

EclipseModel::EclipseModel(const WidgetContext&, QObject* parent)
    : WidgetModel("eclipse", "Next eclipse", parent) {
    timer_.setInterval(60 * 1000);
    connect(&timer_, &QTimer::timeout, this, &EclipseModel::recompute);
}

void EclipseModel::start() { recompute(); timer_.start(); }

void EclipseModel::recompute() {
    s_ = summarize_eclipse(UtcSeconds{now().toSecsSinceEpoch()});
    setState("live", "Computed offline from the system clock (Meeus ch. 54); worldwide, not visibility at your station", now());
    emit changed();
}

void register_widget_eclipse(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new EclipseModel(context);
    registry.add({"eclipse", "Next eclipse", QUrl("qrc:/widgets/eclipse/EclipseCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &EclipseModel::start);
}

} // namespace hamclock
