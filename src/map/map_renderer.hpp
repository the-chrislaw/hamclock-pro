#pragma once
// Pure QPainter map renderer (QtGui only, no QtWidgets): land, coast, night
// shade, subsolar point and DE/DX path. Shared by the Qt Quick MapItem and
// usable on iPad/macOS without the widgets module.
#include "../domain/geometry.hpp"
#include "../domain/solar_position.hpp"
#include "viewport.hpp"
#include <QPainterPath>
#include <QSizeF>
#include <optional>
#include <vector>
class QPainter;
namespace hamclock {
struct MapScene {
    std::optional<Location> de, dx;
    std::vector<Location> route; // short path, empty when none
    bool route_visible = true;
    std::optional<SubsolarPoint> sun;
    bool dark = false;
};
class MapRenderer {
public:
    MapRenderer();
    // Fills the whole size; see MapViewport::fill.
    void paint(QPainter& painter, QSizeF size, const MapScene& scene) const;
    static MapViewport viewportFor(QSizeF size, const MapScene& scene);
private:
    QPainterPath coast_;
    std::vector<QPainterPath> land_;
};
}
