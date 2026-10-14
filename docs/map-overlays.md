# Map overlay API (area G)

Other areas draw on the map through `src/map/overlay.hpp` (QtGui only). Never edit the renderer.

- `OverlayMarker`: `where` (lat/lon), `call`, `prefix`, `color`, `shape` (Dot/Square/Diamond/Triangle), `size_px`, `path_from_de` (dashed short great-circle path from DE; skipped for antipodal or invalid points).
- `OverlayLayer`: `id`, `markers`, `labels` (`None`, `Prefix`, `Call`, `Dot`), `min_label_distance_px`, `visible`. Markers are always drawn; `None` and `Dot` draw no text. A label is dropped when its anchor is closer than `min_label_distance_px` to an earlier kept label of the same layer, so put the most important markers first.
- `MapOverlays::instance().set_layer(layer)` replaces the layer with the same `id`; `remove_layer(id)` removes it. Call from the GUI thread. Layers draw in publish order above DE/DX.
- Publish only real data. When a feed is stale or offline, remove the layer or mark it so in your pane; never keep invented markers.

Worked example (a spots widget):

```cpp
#include "map/overlay.hpp"
hamclock::OverlayLayer layer;
layer.id = "dx-cluster";
layer.labels = hamclock::OverlayLabelMode::Prefix;
layer.min_label_distance_px = 36;
for (const auto& spot : spots) {
    hamclock::OverlayMarker m;
    m.where = {spot.latitude, spot.longitude};
    m.call = spot.call; m.prefix = spot.prefix;
    m.color = QColor("#d1495b"); m.path_from_de = true;
    layer.markers.push_back(m);
}
hamclock::MapOverlays::instance().set_layer(layer);
```

Not yet: tap hit-testing, per-layer legend, projection-aware placement (arrives with G4; markers use the current equirectangular viewport).
