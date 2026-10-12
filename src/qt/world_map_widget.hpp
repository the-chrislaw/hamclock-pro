#pragma once
#include "../domain/clock_state.hpp"
#include "../domain/geometry.hpp"
#include "../domain/solar_position.hpp"
#include <QColor>
#include <QLineF>
#include <QPainterPath>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QWidget>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace hamclock {
class WorldMapWidget : public QWidget {
public:
    explicit WorldMapWidget(QWidget* parent = nullptr);
    void setEndpoints(Location de, Location dx);
    void clearEndpoints();
    void setAppearance(const std::string& appearance);
    void setRouteVisible(bool visible);
    // Shade the night side and mark the subsolar point for this instant.
    // Computed offline; no shade is drawn until a time is set.
    void setSolarTime(UtcSeconds utc);
    void clearSolarTime();
    std::optional<QPointF> subsolarPixel() const;
    QPolygonF nightPolygon() const; // widget pixels; empty when no time set
    QRectF mapBounds() const;
    QPointF endpointPixel(Location location) const;
    std::vector<QLineF> routeSegments() const;
    QString routeSummary() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent*) override;
private:
    void describe();
    QPainterPath coast_;
    std::vector<QPainterPath> land_;
    std::optional<Location> de_, dx_;
    std::vector<Location> route_;
    QString summary_;
    std::optional<SubsolarPoint> sun_;
    std::int64_t solar_minute_ = 0;
    std::string appearance_ = "system";
    bool route_visible_ = true;
};
}
