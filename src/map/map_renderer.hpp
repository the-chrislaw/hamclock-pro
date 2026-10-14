#pragma once
// Pure QPainter map renderer (QtGui only, no QtWidgets): land, coast, night
// shade, subsolar point and DE/DX path. Shared by the Qt Quick MapItem and
// usable on iPad/macOS without the widgets module.
#include "../domain/geometry.hpp"
#include "../domain/solar_position.hpp"
#include "grid.hpp"
#include "overlay.hpp"
#include "projection.hpp"
#include "styles.hpp"
#include "viewport.hpp"
#include <QColor>
#include <QImage>
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
    MapGrid grid = MapGrid::None;
    bool gray = false;                  // grayscale base map
    bool sky_points = false;            // draw sublunar point and DE antipode
    std::optional<Location> moon;       // sublunar point
    std::optional<double> center_lon;   // fixed centre longitude; unset = DE
    MapProjection projection = MapProjection::Equirectangular; // Azimuthal/Robinson ignore zoom and pan
    double zoom = 1;                    // 1..4; >1 centres on DE unless panned
    std::optional<double> pan_lon, pan_lat; // user pan; unset = follow DE/DX
};
class MapRenderer {
public:
    MapRenderer();
    // Fills the whole size; see MapViewport::fill.
    void paint(QPainter& painter, QSizeF size, const MapScene& scene) const;
    static MapViewport viewportFor(QSizeF size, const MapScene& scene);
    // Frame of a whole-world projection (empty, w == 0, for Equirectangular).
    static MapFrame frameFor(QSizeF size, const MapScene& scene);
private:
    QImage equirect_base(const MapScene& scene, const QColor& sea, const QColor& land, const QColor& coast) const;
    void paint_projected(QPainter& painter, QSizeF size, const MapScene& scene, const MapFrame& frame, const QColor& sea,
                         const QColor& land, const QColor& coast, const QColor& route) const;
    mutable QImage base_, proj_; // caches: equirectangular base, projected raster
    mutable QString base_key_, proj_key_;
    QPainterPath coast_;
    std::vector<QPainterPath> land_;
};
}
