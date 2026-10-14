#include "adif_overlay.hpp"
#include "../../domain/locator_input.hpp"
#include <QSet>
#include <exception>

namespace hamclock::adif {

OverlayLayer overlay_layer(const std::vector<Qso>& qsos, int max_markers) {
    OverlayLayer layer;
    layer.id = kOverlayLayerId;
    layer.labels = OverlayLabelMode::None;
    layer.min_label_distance_px = 48;
    QSet<QString> seen;
    for (const auto& q : qsos) {
        if (int(layer.markers.size()) >= max_markers) break;
        const QString call = base_call(q.call);
        if (seen.contains(call)) continue;
        QString grid = q.grid.left(6);
        if (grid.size() == 5) grid.chop(1);
        if (grid.size() != 2 && grid.size() != 4 && grid.size() != 6) continue;
        Location corner{0, 0};
        try { corner = location_from_maidenhead(grid.toStdString()); } catch (const std::exception&) { continue; }
        seen.insert(call);
        // Centre of the cell: 20x10 deg field, 2x1 deg square, 5'x2.5' subsquare.
        const double w = grid.size() == 2 ? 20.0 : grid.size() == 4 ? 2.0 : 5.0 / 60.0;
        const double h = grid.size() == 2 ? 10.0 : grid.size() == 4 ? 1.0 : 2.5 / 60.0;
        OverlayMarker m;
        m.where = {corner.latitude + h / 2, corner.longitude + w / 2};
        m.call = q.call;
        m.color = QColor("#8a6fdf");
        m.shape = OverlayShape::Square;
        m.size_px = 3;
        m.detail = QString("%1 · %2%3 · %4").arg(q.call, q.band.isEmpty() ? "?" : q.band,
                                                  q.mode.isEmpty() ? QString() : " " + q.mode, grid);
        layer.markers.push_back(m);
    }
    return layer;
}

} // namespace hamclock::adif
