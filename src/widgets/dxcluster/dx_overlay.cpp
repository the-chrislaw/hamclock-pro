#include "dx_overlay.hpp"
#include "dxcc_table.hpp"
#include <QSet>

namespace hamclock::dxcluster {

EntityResolver bundled_entity_resolver() {
    return [](const QString& call) -> std::optional<EntityPoint> {
        const auto* e = DxccTable::bundled().lookup(call);
        if (!e) return std::nullopt;
        return EntityPoint{e->lat, e->lon, e->primary_prefix};
    };
}

OverlayLayer overlay_layer(const std::deque<DxSpot>& spots, const EntityResolver& resolve, int max_markers) {
    OverlayLayer layer;
    layer.id = kOverlayLayerId;
    layer.labels = OverlayLabelMode::Prefix;
    layer.min_label_distance_px = 36;
    QSet<QString> seen;
    for (const auto& s : spots) {
        if (int(layer.markers.size()) >= max_markers) break;
        const auto p = resolve ? resolve(s.dx) : std::nullopt;
        if (!p || !qIsFinite(p->lat) || !qIsFinite(p->lon)) continue;
        if (seen.contains(p->prefix)) continue;
        seen.insert(p->prefix);
        OverlayMarker m;
        m.where = {p->lat, p->lon};
        m.call = s.dx;
        m.prefix = p->prefix;
        m.color = QColor("#d1495b");
        m.size_px = 5;
        m.path_from_de = true;
        layer.markers.push_back(m);
    }
    return layer;
}

} // namespace hamclock::dxcluster
