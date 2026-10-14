#include "map_item.hpp"
#include "app_settings.hpp"
#include "../map/path.hpp"
#include <QDateTime>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <exception>

namespace hamclock {
namespace {
AppSettings* g_settings = nullptr;
constexpr int kSolarRefreshMs = 10000; // also steps style rotation (min 20 s);
}

void MapItem::setSettings(AppSettings* settings) { g_settings = settings; }

MapItem::MapItem(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setOpaquePainting(true);
    setAcceptedMouseButtons(Qt::LeftButton);
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
    scene_.overlays = MapOverlays::instance().layers();
    auto sync_style = [this] {
        const auto* st = MapStyles::instance().active(QDateTime::currentMSecsSinceEpoch());
        scene_.style = st ? std::optional<MapStyle>(*st) : std::nullopt;
    };
    sync_style();
    style_token_ = MapStyles::instance().on_change([this, sync_style] { sync_style(); update(); });
    // The registry is process-wide; the callback only ever runs on the GUI thread.
    overlay_token_ = MapOverlays::instance().on_change([this] { scene_.overlays = MapOverlays::instance().layers(); update(); });
    minute_timer_.setInterval(kSolarRefreshMs);
    connect(&minute_timer_, &QTimer::timeout, this, [this, refresh_sun, sync_style] { sync_style(); refresh_sun(); });
    minute_timer_.start();
    connect(this, &QQuickItem::widthChanged, this, [this] { update(); });
    connect(this, &QQuickItem::heightChanged, this, [this] { update(); });
}

MapItem::~MapItem() { MapOverlays::instance().remove_listener(overlay_token_); MapStyles::instance().remove_listener(style_token_); }

void MapItem::setDark(bool dark) {
    if (dark == scene_.dark) return;
    scene_.dark = dark;
    emit darkChanged();
    update();
}

void MapItem::setUiScale(double s) {
    if (s == scene_.ui_scale || s <= 0) return;
    scene_.ui_scale = s;
    emit uiScaleChanged();
    update();
}

void MapItem::setZoom(double z) {
    z = std::clamp(z, 1.0, MapViewport::max_zoom);
    if (z == scene_.zoom) return;
    // Zooming in keeps the current centre so the view does not jump.
    if (z > 1.0 && !scene_.pan_lon) {
        const auto vp = MapRenderer::viewportFor(QSizeF(width(), height()), scene_);
        scene_.pan_lon = vp.center_lon;
        scene_.pan_lat = vp.center_lat;
    }
    scene_.zoom = z;
    if (z <= 1.0) { scene_.pan_lon.reset(); scene_.pan_lat.reset(); }
    emit zoomChanged();
    update();
}

void MapItem::resetView() {
    scene_.pan_lon.reset();
    scene_.pan_lat.reset();
    setZoom(1.0);
    update();
}

void MapItem::panBy(double dx, double dy) {
    if (scene_.zoom <= 1.0 || width() <= 0 || height() <= 0) return;
    const auto vp = MapRenderer::viewportFor(QSizeF(width(), height()), scene_);
    if (vp.scale <= 0) return;
    scene_.pan_lon = vp.center_lon - dx / vp.scale;
    scene_.pan_lat = vp.center_lat + dy / vp.scale;
    update();
}

void MapItem::mousePressEvent(QMouseEvent* e) { press_ = last_ = e->position(); dragging_ = false; }

void MapItem::mouseMoveEvent(QMouseEvent* e) {
    if (!dragging_ && (e->position() - press_).manhattanLength() > 10) dragging_ = true;
    if (dragging_) panBy(e->position().x() - last_.x(), e->position().y() - last_.y());
    last_ = e->position();
}

void MapItem::mouseReleaseEvent(QMouseEvent* e) {
    if (dragging_ || width() <= 0 || height() <= 0) return;
    const auto vp = MapRenderer::viewportFor(QSizeF(width(), height()), scene_);
    if (vp.scale <= 0) return;
    const auto loc = vp.location_at(e->position().x(), e->position().y());
    emit tapped(loc.lat, loc.lon);
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
