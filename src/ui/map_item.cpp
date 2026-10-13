#include "map_item.hpp"
#include "app_settings.hpp"
#include "../map/path.hpp"
#include <QDateTime>
#include <QPainter>
#include <exception>

namespace hamclock {
namespace {
AppSettings* g_settings = nullptr;
constexpr int kSolarRefreshMs = 30000;
}

void MapItem::setSettings(AppSettings* settings) { g_settings = settings; }

MapItem::MapItem(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setOpaquePainting(true);
    if (g_settings) {
        connect(g_settings, &AppSettings::stationChanged, this, &MapItem::syncStation);
        syncStation();
    }
    auto refresh_sun = [this] {
        // The Sun moves 0.25 degrees a minute; the minute-rounded position is plenty.
        scene_.sun = subsolar_point({QDateTime::currentSecsSinceEpoch() / 60 * 60});
        update();
    };
    refresh_sun();
    minute_timer_.setInterval(kSolarRefreshMs);
    connect(&minute_timer_, &QTimer::timeout, this, refresh_sun);
    minute_timer_.start();
    connect(this, &QQuickItem::widthChanged, this, [this] { update(); });
    connect(this, &QQuickItem::heightChanged, this, [this] { update(); });
}

MapItem::~MapItem() = default;

void MapItem::setDark(bool dark) {
    if (dark == scene_.dark) return;
    scene_.dark = dark;
    emit darkChanged();
    update();
}

void MapItem::setShowPath(bool show) {
    if (show == scene_.route_visible) return;
    scene_.route_visible = show;
    emit showPathChanged();
    update();
}

void MapItem::syncStation() {
    scene_.de.reset();
    scene_.dx.reset();
    scene_.route.clear();
    if (g_settings && g_settings->configured()) {
        try {
            const auto& station = g_settings->station();
            const auto result = geometry(station.de, station.dx);
            scene_.de = station.de;
            scene_.dx = station.dx;
            if (result.status != BearingStatus::Antipodal) scene_.route = map_short_path(station.de, station.dx);
        } catch (const std::exception&) {
            scene_.de.reset();
            scene_.dx.reset();
            scene_.route.clear();
        }
    }
    update();
}

void MapItem::paint(QPainter* painter) {
    if (width() <= 0 || height() <= 0) return;
    renderer_.paint(*painter, QSizeF(width(), height()), scene_);
}

} // namespace hamclock
