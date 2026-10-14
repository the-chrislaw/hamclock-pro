#include "map_item.hpp"
#include "../map/sky_points.hpp"
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
        const UtcSeconds now{QDateTime::currentSecsSinceEpoch() / 60 * 60};
        scene_.sun = subsolar_point(now);
        const auto mp = sublunar_point(now);
        scene_.moon = Location{mp.latitude_deg, mp.longitude_deg};
        update();
    };
    refresh_sun();
    if (g_settings) options_.load(g_settings->optionsPath());
    scene_.route_visible = options_.path;
    scene_.night_shade = options_.night_shade;
    scene_.grid = static_cast<MapGrid>(options_.grid);
    scene_.projection = static_cast<MapProjection>(options_.projection);
    scene_.sky_points = options_.sky_points;
    scene_.gray = options_.gray;
    if (options_.center_lon != MapViewOptions::center_auto) scene_.center_lon = options_.center_lon;
    syncOverlays();
    applyStyleOptions();
    auto sync_style = [this] {
        const auto* st = MapStyles::instance().active(QDateTime::currentMSecsSinceEpoch());
        scene_.style = st ? std::optional<MapStyle>(*st) : std::nullopt;
    };
    sync_style();
    style_token_ = MapStyles::instance().on_change([this, sync_style] { sync_style(); update(); emit styleChanged(); });
    // The registry is process-wide; the callback only ever runs on the GUI thread.
    overlay_token_ = MapOverlays::instance().on_change([this] { syncOverlays(); emit layersChanged(); });
    minute_timer_.setInterval(kSolarRefreshMs);
    connect(&minute_timer_, &QTimer::timeout, this, [this, refresh_sun, sync_style] {
        // Styles published after start join the rotation.
        if (options_.rotation_s > 0 && MapStyles::instance().styles().size() != rotation_count_) applyStyleOptions();
        sync_style(); refresh_sun(); });
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
    if (scene_.projection != MapProjection::Equirectangular) return;
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
    const double radius = 14 * std::max(1.0, scene_.ui_scale);
    if (scene_.projection != MapProjection::Equirectangular) {
        const auto frame = MapRenderer::frameFor(QSizeF(width(), height()), scene_);
        const auto hit = hit_test_marker(scene_.overlays, frame, e->position().x(), e->position().y(), radius);
        if (hit.marker) {
            emit markerTapped(hit.layer->id, hit.marker->call, hit.marker->detail, hit.marker->where.latitude, hit.marker->where.longitude);
            return;
        }
        if (const auto loc = frame.to_location(e->position().x(), e->position().y())) emit tapped(loc->latitude, loc->longitude);
        return; // outside the outline: nothing to set
    }
    const auto vp = MapRenderer::viewportFor(QSizeF(width(), height()), scene_);
    if (vp.scale <= 0) return;
    const auto hit = hit_test_marker(scene_.overlays, vp, e->position().x(), e->position().y(), radius);
    if (hit.marker) {
        emit markerTapped(hit.layer->id, hit.marker->call, hit.marker->detail, hit.marker->where.latitude, hit.marker->where.longitude);
        return;
    }
    const auto loc = vp.location_at(e->position().x(), e->position().y());
    emit tapped(loc.lat, loc.lon);
}

void MapItem::syncOverlays() {
    scene_.overlays = options_.apply(MapOverlays::instance().layers());
    update();
}

void MapItem::saveOptions() {
    if (g_settings && !options_.save(g_settings->optionsPath())) qWarning("Could not save the map view options.");
}

QStringList MapItem::layerIds() const {
    QStringList ids;
    for (const auto& l : MapOverlays::instance().layers()) ids << l.id;
    return ids;
}

void MapItem::setLayerVisible(const QString& id, bool on) {
    if (options_.layer_visible(id) == on) return;
    options_.set_layer_visible(id, on);
    saveOptions();
    syncOverlays();
}

void MapItem::setNightShade(bool on) {
    if (on == options_.night_shade) return;
    options_.night_shade = scene_.night_shade = on;
    saveOptions();
    emit nightShadeChanged();
    update();
}

void MapItem::setSkyPoints(bool on) {
    if (on == options_.sky_points) return;
    options_.sky_points = scene_.sky_points = on;
    saveOptions();
    emit skyPointsChanged();
    update();
}

void MapItem::setGray(bool on) {
    if (on == options_.gray) return;
    options_.gray = scene_.gray = on;
    saveOptions();
    emit grayChanged();
    update();
}

void MapItem::setLongPath(bool on) {
    if (on == options_.long_path) return;
    options_.long_path = on;
    saveOptions();
    syncStation();
    emit longPathChanged();
}

void MapItem::setCenterLon(int lon) {
    if (lon != MapViewOptions::center_auto) lon = std::clamp(lon, -180, 180);
    if (lon == options_.center_lon) return;
    options_.center_lon = lon;
    if (lon == MapViewOptions::center_auto) scene_.center_lon.reset(); else scene_.center_lon = lon;
    resetView();
    saveOptions();
    emit centerLonChanged();
    update();
}

void MapItem::setGrid(int g) {
    g = std::clamp(g, 0, map_grid_count - 1);
    if (g == options_.grid) return;
    options_.grid = g;
    scene_.grid = static_cast<MapGrid>(g);
    saveOptions();
    emit gridChanged();
    update();
}

void MapItem::setProjection(int p) {
    p = std::clamp(p, 0, map_projection_count - 1);
    if (p == options_.projection) return;
    options_.projection = p;
    scene_.projection = static_cast<MapProjection>(p);
    resetView(); // zoom and pan only exist on the flat map
    saveOptions();
    emit projectionChanged();
    update();
}

void MapItem::setLabelMode(int mode) {
    mode = std::clamp(mode, -1, static_cast<int>(OverlayLabelMode::Dot));
    if (mode == options_.label_mode) return;
    options_.label_mode = mode;
    saveOptions();
    syncOverlays();
    emit labelModeChanged();
}

// The registry is process-wide; the saved choice is pushed into it. Styles that are
// not published (yet) are skipped by the registry, never faked.
void MapItem::applyStyleOptions() {
    auto& reg = MapStyles::instance();
    reg.select(options_.style);
    std::vector<QString> ids;
    if (options_.rotation_s > 0)
        for (const auto& st : reg.styles()) ids.push_back(st.id);
    rotation_count_ = ids.size();
    reg.set_rotation(options_.rotation_s, std::move(ids));
}

QStringList MapItem::styleIds() const {
    QStringList ids;
    for (const auto& st : MapStyles::instance().styles()) ids << st.id;
    return ids;
}

QVariantList MapItem::rotationChoices() const {
    QVariantList out;
    for (int s : style_rotation_intervals()) out << s;
    return out;
}

void MapItem::setStyle(const QString& id) {
    if (id == options_.style) return;
    options_.style = id;
    saveOptions();
    applyStyleOptions();
}

void MapItem::setRotation(int seconds) {
    const auto& allowed = style_rotation_intervals();
    if (seconds != 0 && std::find(allowed.begin(), allowed.end(), seconds) == allowed.end()) seconds = 0;
    if (seconds == options_.rotation_s) return;
    options_.rotation_s = seconds;
    saveOptions();
    applyStyleOptions();
}

void MapItem::setShowPath(bool show) {
    if (show == scene_.route_visible) return;
    scene_.route_visible = options_.path = show;
    saveOptions();
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
            if (result.status != BearingStatus::Antipodal) scene_.route = options_.long_path ? map_long_path(station.de, station.dx)
                                                                  : map_short_path(station.de, station.dx);
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
