#include "styles.hpp"
#include <algorithm>
namespace hamclock {
QColor color_at(const ColorScale& s, double v) {
    if (s.stops.empty()) return {};
    if (v <= s.stops.front().value) return s.stops.front().color;
    for (std::size_t i = 1; i < s.stops.size(); ++i) {
        const auto &a = s.stops[i - 1], &b = s.stops[i];
        if (v > b.value) continue;
        const double span = b.value - a.value, t = span > 0 ? (v - a.value) / span : 1.0;
        auto mix = [t](int x, int y) { return int(x + (y - x) * t + 0.5); };
        return QColor(mix(a.color.red(), b.color.red()), mix(a.color.green(), b.color.green()),
                      mix(a.color.blue(), b.color.blue()), mix(a.color.alpha(), b.color.alpha()));
    }
    return s.stops.back().color;
}
const std::vector<int>& style_rotation_intervals() { static const std::vector<int> v{0, 20, 60, 90, 120}; return v; }
QString rotation_pick(const std::vector<QString>& ids, int interval_s, qint64 elapsed_ms) {
    if (ids.empty() || interval_s <= 0) return {};
    const qint64 step = std::max<qint64>(0, elapsed_ms) / (qint64(interval_s) * 1000);
    return ids[std::size_t(step % qint64(ids.size()))];
}
QString space_weather_style_id(const SpaceWeatherNow& now) {
    if (now.kp && *now.kp >= 5.0) return "aurora";
    if (now.xray_flux_wm2 && *now.xray_flux_wm2 >= 1e-5) return "drap";
    return {};
}
MapStyles& MapStyles::instance() { static MapStyles s; return s; }
const MapStyle* MapStyles::find(const QString& id) const {
    for (const auto& s : styles_) if (s.id == id) return &s;
    return nullptr;
}
void MapStyles::set_style(MapStyle style) {
    for (auto& s : styles_) if (s.id == style.id) { s = std::move(style); notify(); return; }
    styles_.push_back(std::move(style));
    notify();
}
void MapStyles::remove_style(const QString& id) {
    const auto it = std::remove_if(styles_.begin(), styles_.end(), [&](const MapStyle& s) { return s.id == id; });
    if (it == styles_.end()) return;
    styles_.erase(it, styles_.end());
    notify();
}
void MapStyles::select(const QString& id) { if (id == selected_) return; selected_ = id; notify(); }
void MapStyles::set_rotation(int interval_s, std::vector<QString> ids) { rotation_s_ = interval_s; rotation_ids_ = std::move(ids); notify(); }
void MapStyles::set_auto_space_weather(bool on, SpaceWeatherNow now) { auto_ = on; weather_ = now; notify(); }
const MapStyle* MapStyles::active(qint64 now_ms) const {
    // Only ids that are registered take part, so a missing feed is skipped, not faked.
    std::vector<QString> live;
    for (const auto& id : rotation_ids_) if (find(id)) live.push_back(id);
    if (const auto* s = find(rotation_pick(live, rotation_s_, now_ms))) return s;
    if (auto_) if (const auto* s = find(space_weather_style_id(weather_))) return s;
    return find(selected_);
}
int MapStyles::on_change(std::function<void()> l) { listeners_.emplace_back(next_token_, std::move(l)); return next_token_++; }
void MapStyles::remove_listener(int token) {
    listeners_.erase(std::remove_if(listeners_.begin(), listeners_.end(), [&](const auto& p) { return p.first == token; }), listeners_.end());
}
void MapStyles::notify() const { const auto copy = listeners_; for (const auto& p : copy) p.second(); }
}
