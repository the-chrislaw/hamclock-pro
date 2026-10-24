#include "eclipse_model.hpp"
#include "../../ui/app_settings.hpp"
#include <QDateTime>
#include <QTimeZone>

namespace hamclock {
namespace {
EclipseSummary describe(const SolarEclipse& e, UtcSeconds now) {
    EclipseSummary out;
    out.kind = QString("%1 solar eclipse").arg(solar_eclipse_type_name(e.type));
    out.when = QDateTime::fromSecsSinceEpoch(e.greatest.value, QTimeZone::utc()).toString("d MMM yyyy HH:mm") + "Z";
    const std::int64_t minutes = (e.greatest.value - now.value) / 60;
    const std::int64_t days = minutes / 1440;
    out.countdown = days >= 1 ? QString("in %1 d").arg(days)
                              : QString("in %1 h %2 min").arg(minutes / 60).arg(minutes % 60, 2, 10, QChar('0'));
    out.detail = e.type == SolarEclipseType::Partial
        ? QString("Magnitude %1, greatest eclipse").arg(e.magnitude, 0, 'f', 2)
        : QString("Central, gamma %1").arg(e.gamma, 0, 'f', 2);
    return out;
}
} // namespace

EclipseSummary summarize_eclipse(UtcSeconds now) {
    const auto e = next_solar_eclipse(now);
    return e ? describe(*e, now) : EclipseSummary{};
}

EclipseSummary summarize_eclipse(UtcSeconds now, double lat, double lon) {
    const auto e = next_solar_eclipse(now);
    if (!e) return {};
    EclipseSummary out = describe(*e, now);
    const EclipseLocal loc = eclipse_visibility(*e, lat, lon);
    if (loc.visibility != EclipseVisibility::None)
        out.local = QString("Max %1Z, %2% of Sun")
                        .arg(QDateTime::fromSecsSinceEpoch(loc.max_time.value, QTimeZone::utc()).toString("HH:mm"))
                        .arg(qRound(qMin(loc.magnitude, 1.0) * 100));
    if (loc.start_time.value != 0 && loc.end_time.value != 0) {
        const auto hm = [](UtcSeconds t) { return QDateTime::fromSecsSinceEpoch(t.value, QTimeZone::utc()).toString("HH:mm"); };
        out.contacts = QString("%1%2 – %3%4Z").arg(loc.start_at_sunrise ? QStringLiteral("sunrise ") : QString()).arg(hm(loc.start_time))
                           .arg(loc.end_at_sunset ? QStringLiteral("sunset ") : QString()).arg(hm(loc.end_time));
    }
    if (e->type != SolarEclipseType::Partial && loc.visibility != EclipseVisibility::None) {
        const EclipseContacts c = eclipse_contacts(*e, lat, lon);
        if (c.central && c.sun_up_at_maximum && c.second.value != 0 && c.third.value > c.second.value) {
            const std::int64_t d = c.third.value - c.second.value;
            const auto hms = [](UtcSeconds t) { return QDateTime::fromSecsSinceEpoch(t.value, QTimeZone::utc()).toString("HH:mm:ss"); };
            // The Meeus Sun limits contact times to tens of seconds, so say so.
            out.central = QString("~%1 – %2Z (%3 m %4 s)").arg(hms(c.second)).arg(hms(c.third)).arg(d / 60).arg(d % 60, 2, 10, QChar('0'));
            out.centralLabel = e->type == SolarEclipseType::Annular ? "Annular" : "Totality";
        }
    }
    switch (loc.visibility) {
    case EclipseVisibility::Visible: out.visibility = "Visible at DE"; break;
    case EclipseVisibility::Marginal: out.visibility = "Borderline at DE"; break;
    case EclipseVisibility::None: out.visibility = "Not visible at DE"; break;
    }
    return out;
}

EclipseModel::EclipseModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("eclipse", "Next eclipse", parent), settings_(context.settings) {
    timer_.setInterval(60 * 1000);
    connect(&timer_, &QTimer::timeout, this, &EclipseModel::recompute);
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, &EclipseModel::recompute);
}

void EclipseModel::start() { recompute(); timer_.start(); }

void EclipseModel::recompute() {
    const UtcSeconds t{now().toSecsSinceEpoch()};
    const bool station = settings_ && settings_->configured();
    s_ = station ? summarize_eclipse(t, settings_->deLatitude(), settings_->deLongitude()) : summarize_eclipse(t);
    setState("live", station ? "Computed offline from the system clock (Meeus ch. 54); visibility at DE is approximate"
                             : "Computed offline from the system clock (Meeus ch. 54); worldwide, set your station for visibility",
             now());
    emit changed();
}

void register_widget_eclipse(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new EclipseModel(context);
    registry.add({"eclipse", "Next eclipse", QUrl("qrc:/widgets/eclipse/EclipseCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &EclipseModel::start);
}

} // namespace hamclock
