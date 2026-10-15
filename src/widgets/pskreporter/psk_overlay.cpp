#include "psk_overlay.hpp"
#include "../../domain/geometry.hpp"
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

Farthest farthest_report(const std::vector<Report>& reports, double de_lat, double de_lon) {
    Farthest best;
    for (const auto& r : reports) {
        const int n = r.receiver_grid.size();
        if (n != 4 && n != 6) continue;
        try {
            const Location sw = location_from_maidenhead(r.receiver_grid.toStdString());
            const double dlat = n == 4 ? 1.0 : 2.5 / 60.0, dlon = n == 4 ? 2.0 : 5.0 / 60.0;
            const double km =
                geometry({de_lat, de_lon}, {sw.latitude + dlat / 2, sw.longitude + dlon / 2}).distance_km;
            if (!best.found || km > best.km) best = {true, r.receiver, r.band, km};
        } catch (const std::invalid_argument&) {
        }
    }
    return best;
}

std::vector<Report> within_distance(const std::vector<Report>& reports, double de_lat, double de_lon, double max_km) {
    if (max_km <= 0) return reports;
    std::vector<Report> out;
    for (const auto& r : reports) {
        const int n = r.receiver_grid.size();
        if (n != 4 && n != 6) continue;
        try {
            const Location sw = location_from_maidenhead(r.receiver_grid.toStdString());
            const double dlat = n == 4 ? 1.0 : 2.5 / 60.0, dlon = n == 4 ? 2.0 : 5.0 / 60.0;
            const double km =
                geometry({de_lat, de_lon}, {sw.latitude + dlat / 2, sw.longitude + dlon / 2}).distance_km;
            if (km <= max_km) out.push_back(r);
        } catch (const std::invalid_argument&) {
        }
    }
    return out;
}

std::vector<Report> matching(const std::vector<Report>& reports, const QStringList& tokens) {
    if (tokens.isEmpty()) return reports;
    std::vector<Report> out;
    for (const auto& r : reports)
        for (const QString& t : tokens)
            if (r.receiver.startsWith(t) || r.receiver_grid.toUpper().startsWith(t)) {
                out.push_back(r);
                break;
            }
    return out;
}

} // namespace hamclock::psk
