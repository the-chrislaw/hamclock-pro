// DX cluster map overlay: one marker per DXCC entity, newest first, unknown calls skipped. Offline.
#include "../dx_overlay.hpp"
#include <QCoreApplication>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    using namespace hamclock::dxcluster;

    auto spot = [](const char* dx) { DxSpot s; s.dx = dx; return s; };
    const EntityResolver resolve = [](const QString& call) -> std::optional<EntityPoint> {
        if (call.startsWith("K")) return EntityPoint{39.8, -98.6, "K"};
        if (call.startsWith("DL")) return EntityPoint{51.0, 10.0, "DL"};
        if (call.startsWith("ZZ")) return EntityPoint{std::nan(""), 0, "ZZ"};
        return std::nullopt;
    };
    const std::deque<DxSpot> spots{spot("DL1AA"), spot("K1ABC"), spot("XX9XX"), spot("K2DEF"), spot("ZZ1ZZ")};
    const auto layer = overlay_layer(spots, resolve);
    check(layer.id == "dx-cluster", "layer id");
    check(layer.markers.size() == 2, "unknown, non-finite and repeated-entity spots dropped");
    check(layer.markers.size() == 2 && layer.markers[0].call == "DL1AA" && layer.markers[1].call == "K1ABC", "newest first, newest spot per entity");
    check(layer.markers.size() == 2 && layer.markers[0].where.latitude == 51.0 && layer.markers[0].where.longitude == 10.0, "entity centre");
    check(layer.markers.size() == 2 && layer.markers[0].prefix == "DL" && layer.markers[0].path_from_de, "prefix label and DE path");
    check(layer.labels == OverlayLabelMode::Prefix, "prefix labels");
    check(overlay_layer(spots, resolve, 1).markers.size() == 1, "marker cap");
    check(overlay_layer(spots, nullptr).markers.empty() && overlay_layer({}, resolve).markers.empty(), "no resolver / no spots");
    std::cerr << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
