#pragma once
// Pure QPainter map renderer (QtGui only, no QtWidgets): land, coast, night
// shade, subsolar point and DE/DX path. Shared by the Qt Quick MapItem and
// usable on iPad/macOS without the widgets module.
#include "../domain/geometry.hpp"
#include "../domain/solar_position.hpp"
#include "overlay.hpp"
#include "styles.hpp"
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
    bool night_shade = true;
    std::optional<SubsolarPoint> sun;
    bool dark = false;
    double ui_scale = 1; // Theme.scale: grows labels and markers on TVs
    std::vector<OverlayLayer> overlays; // drawn in order, above DE/DX
    std::optional<MapStyle> style;      // active base-map style image + legend
    double zoom = 1;                    // 1..4; >1 centres on DE unless panned
    std::optional<double> pan_lon, pan_lat; // user pan; unset = follow DE/DX
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
