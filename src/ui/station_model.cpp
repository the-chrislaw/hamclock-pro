#include "station_model.hpp"
#include "app_settings.hpp"
#include "banner_text.hpp"
#include "clock_text.hpp"
#include "../app/station_view.hpp"
#include "../qt/de_time_zone.hpp"
#include <QDateTime>
#include <QTimeZone>
#include <cmath>
#include <exception>

namespace hamclock {
namespace {
constexpr int kTickMs = 1000;

QString coordinates(double latitude, double longitude) {
    return QString("%1° %2, %3° %4")
        .arg(std::abs(latitude), 0, 'f', 2).arg(latitude < 0 ? 'S' : 'N')
        .arg(std::abs(longitude), 0, 'f', 2).arg(longitude < 0 ? 'W' : 'E');
}
} // namespace

StationModel::StationModel(AppSettings& settings, QObject* parent) : QObject(parent), settings_(settings) {
    connect(&settings_, &AppSettings::stationChanged, this, &StationModel::reload);
    connect(&settings_, &AppSettings::optionsChanged, this, &StationModel::stationChanged);
    reload();
    // Align ticks to whole seconds so the displayed second changes on time.
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(kTickMs);
    connect(&timer_, &QTimer::timeout, this, &StationModel::tick);
    QTimer::singleShot(kTickMs - QTime::currentTime().msec(), this, [this] { emit tick(); timer_.start(); });
}

bool StationModel::configured() const { return settings_.configured(); }

QString StationModel::utcTime() const { return QDateTime::currentDateTimeUtc().toString("HH:mm:ss"); }
QString StationModel::utcDate() const { return QDateTime::currentDateTimeUtc().toString("ddd d MMM yyyy"); }

QString StationModel::deTime() const {
    const auto zone = settings_.deZone().toStdString();
    const auto now = QDateTime::currentDateTimeUtc();
    const auto offset = resolve_de_zone_offset(zone, {now.toSecsSinceEpoch()});
    if (!offset) return {};
    return now.addSecs(offset->value).toString("HH:mm");
}

QString StationModel::deClock() const {
    const auto offset = resolve_de_zone_offset(settings_.deZone().toStdString(), {QDateTime::currentSecsSinceEpoch()});
    return offset ? clock_time(QDateTime::currentDateTimeUtc(), offset->value) : QString();
}

QString StationModel::deDate() const {
    const auto offset = resolve_de_zone_offset(settings_.deZone().toStdString(), {QDateTime::currentSecsSinceEpoch()});
    return offset ? clock_date(QDateTime::currentDateTimeUtc(), offset->value) : QString();
}

QString StationModel::deZoneLabel() const {
    const auto zone = settings_.deZone();
    if (zone.isEmpty()) return "Set a time zone in Settings for local time";
    if (!is_valid_de_zone_id(zone.toStdString())) return "Unknown time zone " + zone;
    const QTimeZone tz(zone.toUtf8());
    return zone + " · " + tz.abbreviation(QDateTime::currentDateTimeUtc());
}

QString StationModel::pathText() const {
    if (!has_geometry_) return path_summary(distance_, bearing_);
    return path_detail(geometry_, long_path_, settings_.imperialUnits());
}

QString StationModel::bannerText() const { return banner_text(banner_mode_, callsign_); }

void StationModel::cycleBanner() {
    banner_mode_ = next_banner_mode(banner_mode_);
    emit stationChanged();
}

void StationModel::toggleLongPath() {
    long_path_ = !long_path_;
    emit stationChanged();
}

QString StationModel::deSun() const {
    return configured() ? sun_line(QDateTime::currentSecsSinceEpoch(), de_lat_, de_lon_) : QString();
}
QString StationModel::dxSun() const {
    return configured() ? sun_line(QDateTime::currentSecsSinceEpoch(), dx_lat_, dx_lon_) : QString();
}

void StationModel::reload() {
    if (!settings_.configured()) {
        callsign_ = de_grid_ = dx_grid_ = de_coordinates_ = dx_coordinates_ = distance_ = bearing_ = {};
        has_geometry_ = false;
        emit stationChanged();
        return;
    }
    try {
        const auto view = make_station_view(settings_.station());
        callsign_ = QString::fromStdString(view.callsign);
        de_grid_ = QString::fromStdString(view.de_grid);
        dx_grid_ = QString::fromStdString(view.dx_grid);
        distance_ = QString::fromStdString(view.distance_text);
        bearing_ = QString::fromStdString(view.bearing_text);
        geometry_ = geometry(view.de, view.dx);
        has_geometry_ = true;
        de_lat_ = view.de.latitude; de_lon_ = view.de.longitude;
        dx_lat_ = view.dx.latitude; dx_lon_ = view.dx.longitude;
        de_coordinates_ = coordinates(view.de.latitude, view.de.longitude);
        dx_coordinates_ = coordinates(view.dx.latitude, view.dx.longitude);
    } catch (const std::exception&) {
        distance_ = bearing_ = "Unavailable";
        has_geometry_ = false;
    }
    emit stationChanged();
}

} // namespace hamclock
