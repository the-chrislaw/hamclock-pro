#pragma once
// Overlay API for the map: other areas (spots, satellites, APRS, hazards,
// beacons) publish markers in named layers and never touch the renderer.
// QtGui only. See docs/map-overlays.md.
#include "../domain/geometry.hpp"
#include "projection.hpp"
#include "viewport.hpp"
#include <QColor>
#include <QPointF>
#include <QString>
#include <functional>
#include <utility>
#include <optional>
#include <QRectF>
#include <QSizeF>
#include <vector>
namespace hamclock {
enum class OverlayLabelMode { None, Prefix, Call, Dot };
enum class OverlayShape { Dot, Square, Diamond, Triangle };
struct OverlayMarker {
    Location where{0, 0};
    QString call;   // label for Call mode
    QString prefix; // label for Prefix mode (falls back to call)
    QColor color{"#d1495b"};
    OverlayShape shape = OverlayShape::Dot;
    double size_px = 4;
    bool path_from_de = false; // draw the short great-circle path from DE
    QString detail;            // one line shown when the marker is tapped (optional)
};
struct OverlayLayer {
    QString id;
    std::vector<OverlayMarker> markers;
    OverlayLabelMode labels = OverlayLabelMode::Call;
    // A label is dropped when its anchor is closer than this to an earlier
    // visible label of the same layer. Markers are always drawn.
    double min_label_distance_px = 40;
    bool visible = true;
};
// Text a marker shows in a label mode; empty for None and Dot.
QString overlay_label_text(const OverlayMarker& marker, OverlayLabelMode mode);
// "1234 km, 56° (long 38766 km, 236°)"-style line for the tap popup: distance and
// initial bearing from DE to `where`; empty when DE is unset or the bearing is
// undefined (same place or antipode, distance only then).
QString overlay_path_text(std::optional<Location> de, Location where);
// Greedy, order-preserving: result[i] is true when anchor i keeps its label.
// Label priority of a layer id (lower wins a collision): satellites, then
// spots and everything else, then hazards (quakes, fires, storms).
int overlay_label_priority(const QString& layer_id);
// Display name for a layer id in the map menu ("dx-cluster" -> "DX cluster"); unknown ids
// become sentence case with hyphens as spaces.
QString overlay_layer_name(const QString& layer_id);
// Rects along the polyline `px` (consecutive points closer than `max_gap` are
// joined), so labels can avoid a drawn line.
std::vector<QRectF> line_obstacles(const std::vector<QPointF>& px, double max_gap, double pad);
std::vector<bool> select_labels(const std::vector<QPointF>& anchors, double min_distance_px);

// Rect for a label of `text` beside a marker at `anchor` (radius `marker_r`):
// right of the marker, flipped to the left when it would leave `view`, and
// shifted vertically inside `view`. Empty when it fits neither side or overlaps
// a rect in `used`.
std::optional<QRectF> place_label(QRectF view, QPointF anchor, double marker_r, QSizeF text, const std::vector<QRectF>& used);

// Nearest visible marker within `radius_px` of pixel (px, py), or nullptr.
// Hidden layers never match; the world copies either side of the centre are
// checked so markers near the date line stay tappable.
struct MarkerHit { const OverlayLayer* layer = nullptr; const OverlayMarker* marker = nullptr; };
MarkerHit hit_test_marker(const std::vector<OverlayLayer>& layers, const MapViewport& vp, double px, double py, double radius_px);

// Same for a whole-world projection: markers at their frame pixel position.
MarkerHit hit_test_marker(const std::vector<OverlayLayer>& layers, const MapFrame& frame, double px, double py, double radius_px);

// Registry the app publishes layers into; the map listens for changes.
class MapOverlays {
public:
    static MapOverlays& instance();
    void set_layer(OverlayLayer layer); // replaces a layer with the same id
    void remove_layer(const QString& id);
    const std::vector<OverlayLayer>& layers() const { return layers_; }
    // Returns a token for remove_listener; owners must remove before destruction.
    int on_change(std::function<void()> listener);
    void remove_listener(int token);
private:
    void notify() const;
    std::vector<OverlayLayer> layers_;
    std::vector<std::pair<int, std::function<void()>>> listeners_;
    int next_token_ = 1;
};
}
