#include "map_item.hpp"
#include "app_settings.hpp"
#include "../qt/world_map_widget.hpp"
#include <QDateTime>
#include <QPainter>
#include <exception>

namespace hamclock {
namespace {
AppSettings* g_settings = nullptr;
constexpr int kSolarRefreshMs = 30000;
}

void MapItem::setSettings(AppSettings* settings) { g_settings = settings; }

MapItem::MapItem(QQuickItem* parent) : QQuickPaintedItem(parent), map_(std::make_unique<WorldMapWidget>()) {
    setAntialiasing(true);
    setOpaquePainting(true);
    map_->setAttribute(Qt::WA_DontShowOnScreen);
    map_->setAppearance("light");
    if (g_settings) {
        connect(g_settings, &AppSettings::stationChanged, this, &MapItem::syncStation);
        syncStation();
    }
    auto refresh_sun = [this] {
        map_->setSolarTime({QDateTime::currentSecsSinceEpoch()});
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
    if (dark == dark_) return;
    dark_ = dark;
    map_->setAppearance(dark ? "dark" : "light");
    emit darkChanged();
    update();
}

void MapItem::setShowPath(bool show) {
    if (show == show_path_) return;
    show_path_ = show;
    map_->setRouteVisible(show);
    emit showPathChanged();
    update();
}

void MapItem::syncStation() {
    if (!g_settings || !g_settings->configured()) {
        map_->clearEndpoints();
    } else {
        try {
            map_->setEndpoints(g_settings->station().de, g_settings->station().dx);
        } catch (const std::exception&) {
            map_->clearEndpoints();
        }
    }
    update();
}

void MapItem::paint(QPainter* painter) {
    const QSize size(static_cast<int>(width()), static_cast<int>(height()));
    if (size.isEmpty()) return;
    if (map_->size() != size) map_->resize(size);
    map_->render(painter, QPoint(), QRegion(), QWidget::DrawChildren);
}

} // namespace hamclock
