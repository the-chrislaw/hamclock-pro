#include "psk_overlay.hpp"
#include "../../domain/locator_input.hpp"
#include <QSet>
#include <stdexcept>

namespace hamclock::psk {

OverlayLayer overlay_layer(const std::vector<Report>& reports, int max_markers) {
    OverlayLayer layer;
    layer.id = kOverlayLayerId;
    layer.labels = OverlayLabelMode::Call;
    layer.min_label_distance_px = 44;
    QSet<QString> seen;
    for (const auto& r : reports) {
        if (int(layer.markers.size()) >= max_markers) break;
        const int n = r.receiver_grid.size();
        if ((n != 4 && n != 6) || seen.contains(r.receiver)) continue;
        Location sw{0, 0};
        try {
            sw = location_from_maidenhead(r.receiver_grid.toStdString());
        } catch (const std::invalid_argument&) {
            continue;
        }
        // The locator decodes to its south-west corner; draw at the centre.
        const double dlat = n == 4 ? 1.0 : 2.5 / 60.0, dlon = n == 4 ? 2.0 : 5.0 / 60.0;
        OverlayMarker m;
        m.where = {sw.latitude + dlat / 2, sw.longitude + dlon / 2};
        m.call = r.receiver;
        m.color = QColor("#3b82d6");
        m.shape = OverlayShape::Dot;
        m.size_px = 4;
        m.path_from_de = true;
        seen.insert(r.receiver);
        layer.markers.push_back(m);
    }
    return layer;
}

} // namespace hamclock::psk
