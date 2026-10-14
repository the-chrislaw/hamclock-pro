#pragma once
// Map styles framework: other areas publish a base-map style (an equirectangular
// world image plus a colour-scale legend) and the map shows the active one.
// QtGui only. See docs/map-overlays.md.
#include <QColor>
#include <QImage>
#include <QString>
#include <functional>
#include <optional>
#include <utility>
#include <vector>
namespace hamclock {
struct ColorStop { double value; QColor color; };
struct ColorScale {
    QString title, unit; // e.g. "Kp", "dB"
    std::vector<ColorStop> stops; // ascending by value
};
// Linear blend between stops, clamped at both ends; invalid colour when empty.
QColor color_at(const ColorScale& scale, double value);

struct MapStyle {
    QString id, name;
    QImage image;            // whole world, equirectangular, north up; null = plain base map
    double opacity = 0.6;    // 0..1 over the base map
    ColorScale legend;       // empty stops = no legend
    QString credit;          // short data credit shown with the legend
};

// Rotation intervals offered in settings (seconds); 0 = off.
const std::vector<int>& style_rotation_intervals();
// Which of `ids` is shown after `elapsed_ms` of rotation; empty when ids is empty
// or interval_s is not positive.
QString rotation_pick(const std::vector<QString>& ids, int interval_s, qint64 elapsed_ms);

// Automatic space-weather style: nullopt inputs mean "unknown" and never pick a style.
struct SpaceWeatherNow { std::optional<double> kp, xray_flux_wm2; };
// "aurora" for Kp >= 5, else "drap" for an M-class (>= 1e-5 W/m2) flare, else empty.
QString space_weather_style_id(const SpaceWeatherNow& now);

class MapStyles {
public:
    static MapStyles& instance();
    void set_style(MapStyle style); // replaces by id
    void remove_style(const QString& id);
    const std::vector<MapStyle>& styles() const { return styles_; }
    // Manual choice; "" or an unknown id means the plain base map.
    void select(const QString& id);
    const QString& selected() const { return selected_; }
    void set_rotation(int interval_s, std::vector<QString> ids);
    void set_auto_space_weather(bool on, SpaceWeatherNow now = {});
    // The style to draw at `now_ms` (rotation > auto > manual), nullptr for none.
    const MapStyle* active(qint64 now_ms) const;
    int on_change(std::function<void()> listener);
    void remove_listener(int token);
private:
    void notify() const;
    const MapStyle* find(const QString& id) const;
    std::vector<MapStyle> styles_;
    QString selected_;
    int rotation_s_ = 0;
    std::vector<QString> rotation_ids_;
    bool auto_ = false;
    SpaceWeatherNow weather_;
    std::vector<std::pair<int, std::function<void()>>> listeners_;
    int next_token_ = 1;
};
}
