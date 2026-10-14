#pragma once
// Overlay API for the map: other areas (spots, satellites, APRS, hazards,
// beacons) publish markers in named layers and never touch the renderer.
// QtGui only. See docs/map-overlays.md.
#include "../domain/geometry.hpp"
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
// Greedy, order-preserving: result[i] is true when anchor i keeps its label.
std::vector<bool> select_labels(const std::vector<QPointF>& anchors, double min_distance_px);

// Rect for a label of `text` beside a marker at `anchor` (radius `marker_r`):
// right of the marker, flipped to the left when it would leave `view`, and
// shifted vertically inside `view`. Empty when it fits neither side or overlaps
// a rect in `used`.
std::optional<QRectF> place_label(QRectF view, QPointF anchor, double marker_r, QSizeF text, const std::vector<QRectF>& used);

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
