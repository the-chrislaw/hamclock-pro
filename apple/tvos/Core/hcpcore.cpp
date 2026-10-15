#include "hcpcore.h"
#include "quakes.hpp"
#include "solarwind.hpp"
#include "swpc.hpp"
#include "domain/locator_input.hpp"
#include "domain/astro_moon.hpp"
#include "domain/astro_sun.hpp"
#include "domain/astro_satellite.hpp"
#include "domain/solar_position.hpp"
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
std::string_view view(const char* body, size_t len) { return body ? std::string_view(body, len) : std::string_view(); }
int copy(const std::string& s, char* buf, size_t cap) {
    if (!buf || cap == 0) return 0;
    const size_t n = s.size() < cap - 1 ? s.size() : cap - 1;
    std::memcpy(buf, s.data(), n);
    buf[n] = '\0';
    return n > 0;
}
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
    if (!t || !out || obs_lat < -90 || obs_lat > 90 || obs_lon < -180 || obs_lon > 360) return 0;
    const auto sets = hamclock::parse_tle_set(std::string(t, n));
    if (index < 0 || static_cast<size_t>(index) >= sets.size()) return 0;
    const hamclock::SatellitePropagator p(sets[static_cast<size_t>(index)]);
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
}
