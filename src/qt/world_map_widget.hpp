#pragma once
#include "../domain/geometry.hpp"
#include <QColor>
#include <QLineF>
#include <QPainterPath>
#include <QRectF>
#include <QWidget>
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
    std::string appearance_ = "system";
    bool route_visible_ = true;
};
}
