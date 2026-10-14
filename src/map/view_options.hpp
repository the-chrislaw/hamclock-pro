#pragma once
// Map View menu state (QtCore only): which overlay layers are shown, the label
// mode override, night shading and the DE/DX path. Layers are named by the
// owning areas; the user choice is stored by layer id so it survives restarts
// even while a layer is not yet published. Saved in options.ini ([map]).
#include "grid.hpp"
#include "overlay.hpp"
#include <QSet>
#include <QString>
#include <QStringList>
#include <vector>
namespace hamclock {
struct MapViewOptions {
    QSet<QString> hidden_layers;
    int label_mode = -1; // -1 = each layer's own mode, else OverlayLabelMode
    bool night_shade = true;
    bool path = true;
    bool long_path = false; // draw the DE/DX path the long way round
    QString style;        // base style id; "" = plain base map
    int grid = 0;         // MapGrid
    bool gray = false;       // grayscale base map (land, sea, coast)
    bool sky_points = false; // sublunar point and DE antipode markers
    static constexpr int center_auto = 999; // centre on DE
    int center_lon = center_auto; // map centre longitude (-180..180) or center_auto
    int rotation_s = 0;   // style rotation interval in seconds, 0 = off

    bool layer_visible(const QString& id) const { return !hidden_layers.contains(id); }
    void set_layer_visible(const QString& id, bool on);
    // Copy of `layers` with visibility and label override applied.
    std::vector<OverlayLayer> apply(const std::vector<OverlayLayer>& layers) const;
    void load(const QString& ini_path);
    // Returns false when the file could not be written.
    bool save(const QString& ini_path) const;
};
}
