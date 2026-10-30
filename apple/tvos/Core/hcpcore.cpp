#include "hcpcore.h"
#include "abi_util.hpp"
#include "bands.hpp"
#include "quakes.hpp"
#include "storms.hpp"
#include "solarwind.hpp"
#include "swpc.hpp"
#include "satpass.hpp"
#include "weather.hpp"
#include "map/coastline.hpp"
#include "domain/locator_input.hpp"
#include "domain/astro_moon.hpp"
#include "domain/astro_sun.hpp"
#include "domain/astro_satellite.hpp"
#include "domain/solar_position.hpp"
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <cstring>
#include <string>
#include <string_view>

namespace {
HcpReading wrap(const std::optional<hcp::swpc::Reading>& r) {
    HcpReading out{};
    if (r) { out.valid = 1; out.value = r->value; out.time_ms = r->time_ms; }
    return out;
}
HcpReading wrap(const std::optional<hcp::solarwind::Point>& p) {
    HcpReading out{};
    if (p) { out.valid = 1; out.value = p->value; out.time_ms = p->time_ms; }
    return out;
}
using hcp::abi::copy;
using hcp::abi::view;
} // namespace

extern "C" {
HcpReading hcp_swpc_latest_kp(const char* b, size_t n) { return wrap(hcp::swpc::latest_kp(view(b, n))); }
HcpReading hcp_swpc_latest_solar_flux(const char* b, size_t n) { return wrap(hcp::swpc::latest_solar_flux(view(b, n))); }
HcpReading hcp_swpc_latest_xray(const char* b, size_t n) { return wrap(hcp::swpc::latest_xray_flux(view(b, n))); }
int hcp_swpc_xray_class(double f, char* buf, size_t cap) { return copy(hcp::swpc::xray_class(f), buf, cap); }
int hcp_swpc_kp_description(double k, char* buf, size_t cap) { return copy(hcp::swpc::kp_description(k), buf, cap); }
HcpReading hcp_solarwind_latest_speed(const char* b, size_t n) { return wrap(hcp::solarwind::latest(view(b, n), hcp::solarwind::kSpeed)); }
HcpReading hcp_solarwind_latest_density(const char* b, size_t n) { return wrap(hcp::solarwind::latest(view(b, n), hcp::solarwind::kDensity)); }
HcpReading hcp_solarwind_latest_bt(const char* b, size_t n) { return wrap(hcp::solarwind::latest(view(b, n), hcp::solarwind::kBt)); }
HcpReading hcp_solarwind_latest_bz(const char* b, size_t n) { return wrap(hcp::solarwind::latest(view(b, n), hcp::solarwind::kBz)); }
int hcp_solarwind_speed_description(double v, char* buf, size_t cap) { return copy(hcp::solarwind::speed_description(v), buf, cap); }
int hcp_solarwind_bz_description(double v, char* buf, size_t cap) { return copy(hcp::solarwind::bz_description(v), buf, cap); }
int hcp_subsolar_point(int64_t t, double* lat, double* lon) {
    if (!lat || !lon) return 0;
    const auto p = hamclock::subsolar_point(hamclock::UtcSeconds{t});
    *lat = p.latitude_deg; *lon = p.longitude_deg;
    return 1;
}
int hcp_sun_elevation(int64_t t, double lat, double lon, double* out) {
    if (!out) return 0;
    try {
        *out = hamclock::solar_elevation_deg(hamclock::subsolar_point(hamclock::UtcSeconds{t}), lat, lon);
    } catch (const std::invalid_argument&) { return 0; }
    return 1;
}
int hcp_next_sun_event(int64_t t, int kind, double lat, double lon, int64_t* out) {
    if (!out || (kind != 0 && kind != 1)) return 0;
    try {
        const auto e = hamclock::next_sun_event(hamclock::UtcSeconds{t},
            kind == 0 ? hamclock::SunEventKind::Rise : hamclock::SunEventKind::Set, lat, lon);
        if (!e) return 0;
        *out = e->value;
    } catch (const std::invalid_argument&) { return 0; }
    return 1;
}
int hcp_next_moon_event(int64_t t, int kind, double lat, double lon, int64_t* out) {
    if (!out || (kind != 0 && kind != 1)) return 0;
    try {
        const auto e = hamclock::next_moon_event(hamclock::UtcSeconds{t},
            kind == 0 ? hamclock::MoonEventKind::Rise : hamclock::MoonEventKind::Set, lat, lon);
        if (!e) return 0;
        *out = e->value;
    } catch (const std::invalid_argument&) { return 0; }
    return 1;
}
int hcp_moon_phase(int64_t t, char* buf, size_t cap, double* illuminated) {
    if (!illuminated) return 0;
    const auto p = hamclock::moon_phase(hamclock::UtcSeconds{t});
    *illuminated = p.illuminated;
    return copy(p.name, buf, cap);
}
int hcp_locator_center(const char* loc, double* lat, double* lon) {
    if (!loc || !lat || !lon) return 0;
    try {
        const std::string text(loc);
        const auto sw = hamclock::location_from_maidenhead(text);
        const double dlat = text.size() == 2 ? 10.0 : text.size() == 4 ? 1.0 : 2.5 / 60.0;
        const double dlon = text.size() == 2 ? 20.0 : text.size() == 4 ? 2.0 : 5.0 / 60.0;
        *lat = sw.latitude + dlat / 2; *lon = sw.longitude + dlon / 2;
    } catch (const std::invalid_argument&) { return 0; }
    return 1;
}
}

extern "C" {
int hcp_quakes_parse(const char* b, size_t n, HcpQuake* out, size_t cap) {
    const auto list = hcp::quakes::parse(view(b, n));
    if (!list) return -1;
    size_t i = 0;
    for (; i < list->size() && i < cap && out; ++i) {
        const auto& q = (*list)[i];
        out[i] = HcpQuake{};
        out[i].magnitude = q.magnitude;
        out[i].latitude = q.latitude;
        out[i].longitude = q.longitude;
        out[i].depth_km = q.depth_km;
        out[i].time_ms = q.time_ms;
        copy(q.place, out[i].place, sizeof out[i].place);
    }
    return static_cast<int>(i);
}
int hcp_quakes_strongest_index(const char* b, size_t n) {
    const auto list = hcp::quakes::parse(view(b, n));
    if (!list) return -1;
    const auto* s = hcp::quakes::strongest(*list);
    return s ? static_cast<int>(s - list->data()) : -1;
}
int hcp_quake_magnitude_class(double m, char* buf, size_t cap) { return copy(hcp::quakes::magnitude_class(m), buf, cap); }
}

namespace {
bool valid_observer(double lat, double lon) { return lat >= -90 && lat <= 90 && lon >= -180 && lon <= 360; }

// One TLE set propagated to unix_s; 0 when SGP4 fails (e.g. decayed).
int sat_state(const hamclock::Tle& set, int64_t unix_s, double obs_lat, double obs_lon, HcpSatState* out) {
    const hamclock::SatellitePropagator p(set);
    if (!p.valid()) return 0;
    const double when = static_cast<double>(unix_s);
    hamclock::SubSatellitePoint sp;
    hamclock::LookAngle la;
    bool lit = false;
    if (!p.sub_point(when, sp) || !p.look(when, hamclock::Observer{obs_lat, obs_lon, 0.0}, la) || !p.sunlit(when, lit)) return 0;
    *out = HcpSatState{sp.lat_deg, sp.lon_deg, sp.alt_km, la.az_deg, la.el_deg, la.range_km,
                       la.range_rate_km_s, lit ? 1 : 0, static_cast<int64_t>(p.epoch_unix())};
    return 1;
}
} // namespace

extern "C" {
int hcp_tle_count(const char* t, size_t n) {
    if (!t) return 0;
    return static_cast<int>(hamclock::parse_tle_set(std::string(t, n)).size());
}
int hcp_tle_name(const char* t, size_t n, int index, char* buf, size_t cap) {
    if (!t) return 0;
    const auto sets = hamclock::parse_tle_set(std::string(t, n));
    if (index < 0 || static_cast<size_t>(index) >= sets.size()) return 0;
    return copy(sets[static_cast<size_t>(index)].name, buf, cap);
}
int hcp_sat_state(const char* t, size_t n, int index, int64_t unix_s,
                  double obs_lat, double obs_lon, HcpSatState* out) {
    if (!t || !out || !valid_observer(obs_lat, obs_lon)) return 0;
    const auto sets = hamclock::parse_tle_set(std::string(t, n));
    if (index < 0 || static_cast<size_t>(index) >= sets.size()) return 0;
    return sat_state(sets[static_cast<size_t>(index)], unix_s, obs_lat, obs_lon, out);
}
int hcp_sat_states(const char* t, size_t n, int64_t unix_s, double obs_lat, double obs_lon,
                   HcpSatInfo* out, size_t cap) {
    if (!t || !valid_observer(obs_lat, obs_lon)) return 0;
    const auto sets = hamclock::parse_tle_set(std::string(t, n));
    for (size_t i = 0; out && i < sets.size() && i < cap; ++i) {
        out[i] = HcpSatInfo{};
        copy(sets[i].name, out[i].name, sizeof out[i].name);
        out[i].valid = sat_state(sets[i], unix_s, obs_lat, obs_lon, &out[i].state);
    }
    return static_cast<int>(sets.size());
}
int hcp_hf_outlook(double sfi, double kp, double a_index, HcpBand* out, size_t cap) {
    if (!out) return 0;
    const auto bands = hcp::bands::hf_outlook(sfi, kp, a_index);
    size_t n = 0;
    for (const auto& b : bands) {
        if (n >= cap) break;
        HcpBand& o = out[n++];
        copy(b.band, o.band, sizeof o.band);
        o.mhz = b.mhz;
        o.day = static_cast<int>(b.day);
        o.night = static_cast<int>(b.night);
    }
    return static_cast<int>(n);
}
int hcp_coast_part_count(void) { return static_cast<int>(hamclock::coastline_part_count); }
int hcp_coast_part(int index, uint32_t* start, uint32_t* count) {
    if (!start || !count || index < 0 || static_cast<size_t>(index) >= hamclock::coastline_part_count) return 0;
    const auto& p = hamclock::coastline_parts[index];
    if (static_cast<size_t>(p.start) + p.count > hamclock::coastline_point_count) return 0;
    *start = p.start;
    *count = p.count;
    return 1;
}
const float* hcp_coast_points(size_t* point_count) {
    static_assert(sizeof(hamclock::CoastPoint) == 2 * sizeof(float), "CoastPoint must be two packed floats");
    if (point_count) *point_count = hamclock::coastline_point_count;
    return &hamclock::coastline_points[0].longitude;
}
}

extern "C" {
int hcp_weather_parse(const char* b, size_t n, HcpWeather* out) {
    if (!out) return 0;
    const auto c = hcp::weather::parse_current(view(b, n));
    if (!c) return 0;
    *out = HcpWeather{};
    out->temperature_c = c->temperature_c;
    out->wind_kmh = c->wind_kmh;
    out->humidity_pct = c->humidity_pct;
    out->wind_dir_deg = c->wind_dir_deg;
    out->weather_code = c->weather_code;
    out->is_day = c->is_day ? 1 : 0;
    if (const auto dew = hcp::weather::dew_point_c(c->temperature_c, c->humidity_pct)) {
        out->dew_point_c = *dew;
        out->has_dew_point = 1;
    }
    if (c->pressure_hpa) { out->pressure_hpa = *c->pressure_hpa; out->has_pressure = 1; }
    if (c->pressure_change_hpa) {
        out->pressure_change_hpa = *c->pressure_change_hpa;
        out->has_pressure_change = 1;
        copy(hcp::weather::pressure_trend_word(*c->pressure_change_hpa), out->trend, sizeof out->trend);
    }
    copy(hcp::weather::wmo_description(c->weather_code), out->description, sizeof out->description);
    copy(hcp::weather::compass_point(c->wind_dir_deg), out->wind_from, sizeof out->wind_from);
    return 1;
}
}

extern "C" {
int hcp_storms_parse(const char* b, size_t n, HcpStorm* out, size_t cap) {
    const auto list = hcp::storms::parse(view(b, n));
    if (!list) return -1;
    size_t i = 0;
    for (; i < list->size() && i < cap && out; ++i) {
        const auto& s = (*list)[i];
        out[i] = HcpStorm{};
        copy(s.name, out[i].name, sizeof out[i].name);
        copy(hcp::storms::classification_text(s.classification), out[i].classification, sizeof out[i].classification);
        copy(hcp::storms::compass(s.heading_deg), out[i].heading, sizeof out[i].heading);
        out[i].wind_kt = s.wind_kt;
        out[i].pressure_mb = s.pressure_mb;
        out[i].category = hcp::storms::category(s.wind_kt);
        out[i].speed_kt = s.speed_kt;
        out[i].latitude = s.latitude;
        out[i].longitude = s.longitude;
        out[i].issued_ms = s.issued_ms;
    }
    return static_cast<int>(i);
}
}

extern "C" {
int hcp_sat_passes(const char* t, size_t n, int index, int64_t from_s, int64_t horizon_s,
                   double obs_lat, double obs_lon, double min_el, HcpPass* out, size_t cap) {
    if (!t || !out || obs_lat < -90 || obs_lat > 90 || obs_lon < -180 || obs_lon > 360 || horizon_s <= 0) return -1;
    const auto sets = hamclock::parse_tle_set(std::string(t, n));
    if (index < 0 || static_cast<size_t>(index) >= sets.size()) return -1;
    const auto& tle = sets[static_cast<size_t>(index)];
    if (!hamclock::SatellitePropagator(tle).valid()) return -1;
    constexpr size_t kMaxPasses = 64;
    const int want = static_cast<int>(cap < kMaxPasses ? cap : kMaxPasses);
    const auto passes =
        hcp::sats::next_passes(tle, hamclock::Observer{obs_lat, obs_lon, 0.0}, from_s, horizon_s, min_el, want);
    size_t i = 0;
    for (; i < passes.size() && i < cap; ++i) {
        const auto& p = passes[i];
        out[i] = HcpPass{p.aos, p.tca, p.los, p.max_el_deg, p.aos_az_deg, p.los_az_deg};
    }
    return static_cast<int>(i);
}
int hcp_tle_short_name(const char* t, size_t n, int index, char* buf, size_t cap) {
    if (!t) return 0;
    const auto sets = hamclock::parse_tle_set(std::string(t, n));
    if (index < 0 || static_cast<size_t>(index) >= sets.size()) return 0;
    return copy(hcp::sats::short_name(sets[static_cast<size_t>(index)].name), buf, cap);
}
int hcp_tle_find_short_name(const char* t, size_t n, const char* want) {
    if (!t || !want || !*want) return -1;
    const auto lower = [](std::string s) {
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    const std::string target = lower(want);
    const auto sets = hamclock::parse_tle_set(std::string(t, n));
    for (size_t i = 0; i < sets.size(); ++i)
        if (lower(hcp::sats::short_name(sets[i].name)) == target) return static_cast<int>(i);
    return -1;
}
}
