#include "daylight_model.hpp"
#include "../../ui/app_settings.hpp"
#include <QDate>
#include <QDateTime>
#include <QTimeZone>
#include <QVariantMap>
#include <cmath>

namespace hamclock {

QVariantList daylight_points(int year, double lat, double lon, int step_days, const QString& zone_id) {
    QVariantList out;
    const QTimeZone zone = zone_id.isEmpty() ? QTimeZone() : QTimeZone(zone_id.toUtf8());
    const bool local = zone.isValid();
    if (step_days < 1) step_days = 1;
    const auto series = sun_year_series(year, lat, lon);
    for (std::size_t i = 0; i < series.size(); i += static_cast<std::size_t>(step_days)) {
        double rise = series[i].rise_min, set = series[i].set_min;
        if (local) {
            const QDateTime noon(QDate(year, 1, 1).addDays(series[i].day_of_year - 1), QTime(12, 0), Qt::UTC);
            const double off = zone.offsetFromUtc(noon) / 60.0;
            const auto shift = [off](double m) { return m < 0 ? m : std::fmod(std::fmod(m + off, 1440.0) + 1440.0, 1440.0); };
            rise = shift(rise);
            set = shift(set);
        }
        out.append(QVariantMap{{"day", series[i].day_of_year}, {"rise", rise}, {"set", set}});
    }
    return out;
}

DaylightModel::DaylightModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("daylight", "Daylight year", parent), settings_(context.settings) {
    timer_.setInterval(60 * 60 * 1000);
    connect(&timer_, &QTimer::timeout, this, &DaylightModel::recompute);
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, &DaylightModel::recompute);
}

void DaylightModel::start() { recompute(); timer_.start(); }

void DaylightModel::recompute() {
    const QDate d = now().date();
    days_ = d.daysInYear();
    today_ = d.dayOfYear() - 1;
    has_station_ = settings_ && settings_->configured();
    const QString zone = settings_ ? settings_->deZone() : QString();
    local_ = has_station_ && QTimeZone(zone.toUtf8()).isValid();
    points_ = has_station_ ? daylight_points(d.year(), settings_->deLatitude(), settings_->deLongitude(), 7, zone) : QVariantList{};
    if (has_station_) setState("live", "Computed offline from the system clock (NOAA solar method, about a minute); times in " + QString(local_ ? "DE local time" : "UTC"), now());
    else setState("unconfigured", "Set your station location in Settings");
    emit changed();
}

void register_widget_daylight(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new DaylightModel(context);
    registry.add({"daylight", "Daylight year", QUrl("qrc:/widgets/daylight/DaylightCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &DaylightModel::start);
}

} // namespace hamclock
