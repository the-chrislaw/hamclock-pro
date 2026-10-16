#include "sun_moon_model.hpp"
#include "../../ui/app_settings.hpp"
#include <QDateTime>
#include <QTimeZone>

namespace hamclock {
namespace {
QString hhmm(UtcSeconds t) {
    return QDateTime::fromSecsSinceEpoch(t.value, QTimeZone::utc()).toString("HH:mm") + " UTC";
}
} // namespace

SunSummary summarize_sun(UtcSeconds now, double lat, double lon) {
    SunSummary out;
    const std::int64_t day = now.value - ((now.value % 86400) + 86400) % 86400;
    const auto today = sun_events_for_utc_day(day, lat, lon);
    if (today.polar == PolarState::MidnightSun) out.note = "Midnight sun";
    if (today.polar == PolarState::PolarNight) out.note = "Polar night";
    if (const auto rise = next_sun_event(now, SunEventKind::Rise, lat, lon)) out.next_rise = hhmm(*rise);
    if (const auto set = next_sun_event(now, SunEventKind::Set, lat, lon)) out.next_set = hhmm(*set);
    // Day length: first rise followed by a set within today and tomorrow.
    std::vector<SunEvent> events = today.events;
    for (const auto& e : sun_events_for_utc_day(day + 86400, lat, lon).events) events.push_back(e);
    for (std::size_t i = 0; i + 1 < events.size(); ++i) {
        if (events[i].kind == SunEventKind::Rise && events[i + 1].kind == SunEventKind::Set) {
            const auto minutes = (events[i + 1].time.value - events[i].time.value + 30) / 60;
            out.day_length = QString("%1 h %2 min").arg(minutes / 60).arg(minutes % 60, 2, 10, QChar('0'));
            break;
        }
    }
    if (out.day_length.isEmpty() && today.polar == PolarState::MidnightSun) out.day_length = "24 h 00 min";
    if (out.day_length.isEmpty() && today.polar == PolarState::PolarNight) out.day_length = "0 h 00 min";
    return out;
}

QString summarize_season(UtcSeconds now) {
    static const char* names[] = {"March equinox", "June solstice", "September equinox", "December solstice"};
    const SeasonEvent e = next_season_event(now);
    return QString("%1 %2 UTC").arg(names[static_cast<int>(e.kind)],
        QDateTime::fromSecsSinceEpoch(e.time.value, QTimeZone::utc()).toString("d MMM HH:mm"));
}

MoonSummary summarize_moon(UtcSeconds now, double lat, double lon) {
    MoonSummary out;
    const MoonPhase phase = moon_phase(now);
    out.phase = phase.name;
    out.elongation_deg = phase.elongation_deg;
    out.southern = lat < 0;
    out.tilt_deg = moon_bright_limb_tilt_deg(now, lat, lon);
    out.illumination = QString("%1%").arg(qRound(phase.illuminated * 100));
    if (const auto rise = next_moon_event(now, MoonEventKind::Rise, lat, lon)) out.next_rise = hhmm(*rise);
    if (const auto set = next_moon_event(now, MoonEventKind::Set, lat, lon)) out.next_set = hhmm(*set);
    const MoonHorizontal h = moon_horizontal(now, lat, lon);
    out.position = QString("Az %1° El %2°").arg(qRound(h.azimuth_deg) % 360).arg(qRound(h.elevation_deg));
    return out;
}

SunMoonModel::SunMoonModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("sunmoon", "Sun & Moon", parent), settings_(context.settings) {
    timer_.setInterval(60 * 1000);
    connect(&timer_, &QTimer::timeout, this, &SunMoonModel::recompute);
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, &SunMoonModel::recompute);
}

void SunMoonModel::start() { recompute(); timer_.start(); }

void SunMoonModel::recompute() {
    if (!settings_ || !settings_->configured()) {
        has_station_ = false;
        de_ = dx_ = {};
        moon_ = {};
        season_.clear();
        setState("unconfigured", "Set your station location in Settings");
        emit changed();
        return;
    }
    const UtcSeconds t{now().toSecsSinceEpoch()};
    de_ = summarize_sun(t, settings_->deLatitude(), settings_->deLongitude());
    dx_ = summarize_sun(t, settings_->dxLatitude(), settings_->dxLongitude());
    moon_ = summarize_moon(t, settings_->deLatitude(), settings_->deLongitude());
    season_ = summarize_season(t);
    has_station_ = true;
    setState("live", "Computed offline from the system clock", now());
    emit changed();
}

void register_widget_sunmoon(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SunMoonModel(context);
    registry.add({"sunmoon", "Sun & Moon", QUrl("qrc:/widgets/sunmoon/SunMoonCard.qml"), "strip", 50}, model);
    QTimer::singleShot(0, model, &SunMoonModel::start);
}

} // namespace hamclock
