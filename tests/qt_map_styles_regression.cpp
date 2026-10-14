#include "../src/map/map_renderer.hpp"
#include "../src/map/styles.hpp"
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <iostream>
using namespace hamclock;
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    int failures = 0;
    auto check = [&](bool ok, const char* name) { if (!ok) { ++failures; std::cerr << "Map styles regression failed: " << name << '\n'; } };

    ColorScale sc; sc.stops = {{0, QColor(0, 0, 0)}, {10, QColor(200, 100, 50)}};
    check(color_at(sc, 5) == QColor(100, 50, 25), "midpoint blend");
    check(color_at(sc, -3) == QColor(0, 0, 0) && color_at(sc, 99) == QColor(200, 100, 50), "clamped ends");
    check(!color_at(ColorScale{}, 1).isValid(), "empty scale has no colour");

    check(style_rotation_intervals() == std::vector<int>({0, 20, 60, 90, 120}), "rotation intervals");
    const std::vector<QString> ids{"a", "b", "c"};
    check(rotation_pick(ids, 20, 0) == "a" && rotation_pick(ids, 20, 19999) == "a", "first step");
    check(rotation_pick(ids, 20, 20000) == "b" && rotation_pick(ids, 20, 60000) == "a", "steps and wraps");
    check(rotation_pick(ids, 0, 5000).isEmpty() && rotation_pick({}, 20, 5000).isEmpty(), "off or empty");

    check(space_weather_style_id({5.0, std::nullopt}) == "aurora", "Kp5 -> aurora");
    check(space_weather_style_id({3.0, 2e-5}) == "drap", "M flare -> drap");
    check(space_weather_style_id({5.0, 2e-5}) == "aurora", "aurora wins");
    check(space_weather_style_id({3.0, 1e-6}).isEmpty() && space_weather_style_id({}).isEmpty(), "quiet or unknown -> none");

    auto& reg = MapStyles::instance();
    int calls = 0;
    const int token = reg.on_change([&] { ++calls; });
    MapStyle aurora; aurora.id = "aurora"; aurora.name = "Aurora";
    MapStyle drap; drap.id = "drap"; drap.name = "DRAP";
    reg.set_style(aurora); reg.set_style(drap); reg.set_style(aurora);
    check(reg.styles().size() == 2 && calls == 3, "set_style replaces by id");
    check(reg.active(0) == nullptr, "default is the plain map");
    reg.select("drap");
    check(reg.active(0) && reg.active(0)->id == "drap", "manual selection");
    reg.select("nonexistent");
    check(reg.active(0) == nullptr, "unknown id is the plain map");
    reg.set_auto_space_weather(true, {6.0, std::nullopt});
    check(reg.active(0) && reg.active(0)->id == "aurora", "auto picks aurora");
    reg.set_auto_space_weather(true, {1.0, std::nullopt});
    check(reg.active(0) == nullptr, "auto quiet falls back to manual (plain)");
    reg.set_rotation(20, {"aurora", "missing", "drap"});
    check(reg.active(0)->id == "aurora" && reg.active(20000)->id == "drap", "rotation skips unregistered ids");
    reg.set_rotation(0, {});
    reg.remove_style("aurora"); reg.remove_style("aurora");
    check(reg.styles().size() == 1, "remove_style");
    reg.remove_listener(token);
    const int before = calls; reg.select("drap");
    check(calls == before, "removed listener is silent");

    // Rendering: a styled map differs from the plain one and draws a legend.
    MapScene plain; QImage a(400, 200, QImage::Format_ARGB32), b(400, 200, QImage::Format_ARGB32);
    MapRenderer r;
    { QPainter p(&a); r.paint(p, QSizeF(400, 200), plain); }
    MapScene styled = plain;
    MapStyle st; st.id = "x"; st.opacity = 1.0;
    st.image = QImage(8, 4, QImage::Format_ARGB32); st.image.fill(QColor(255, 0, 0));
    st.legend.title = "Test"; st.legend.stops = {{0, Qt::blue}, {1, Qt::red}};
    styled.style = st;
    { QPainter p(&b); r.paint(p, QSizeF(400, 200), styled); }
    check(a != b, "style image changes the render");
    check(b.pixelColor(200, 100).red() > 200, "style image covers the map");
    return failures == 0 ? 0 : 1;
}
