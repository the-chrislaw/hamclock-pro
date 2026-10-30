// C ABI: planets, solar eclipses, great-circle path, Maidenhead and the land/border map tables (see hcpcore.h).
#include "hcpcore.h"
#include "abi_util.hpp"
#include "domain/astro_eclipse.hpp"
#include "domain/astro_planets.hpp"
#include "domain/geometry.hpp"
#include "map/borders.hpp"
#include "map/land.hpp"
#include <cmath>
#include <stdexcept>
#include <vector>

using hcp::abi::copy;

namespace {
constexpr int64_t kPlanetStepS = 300;            // coarse elevation sampling
constexpr int64_t kPlanetWindowS = 48 * 3600;    // search window
constexpr size_t kLandRingCount = sizeof(hamclock::land_rings) / sizeof(hamclock::land_rings[0]);
constexpr size_t kLandPointCount = sizeof(hamclock::land_points) / sizeof(hamclock::land_points[0]);

bool planet_index(int index, hamclock::Planet* p) {
    if (index < 0 || index >= hamclock::kPlanetCount) return false;
    *p = static_cast<hamclock::Planet>(index);
    return true;
}

double elevation(hamclock::Planet p, int64_t t, double lat, double lon) {
    return hamclock::planet_horizontal(p, hamclock::UtcSeconds{t}, lat, lon).elevation_deg;
}

// True when the horizon state at t is the one the event leads into (up for a rise, down for a set).
bool after_event(bool rise, double el) { return rise ? el >= 0 : el < 0; }

int part(int index, size_t part_count, size_t point_count, uint32_t s, uint32_t c, uint32_t* start, uint32_t* count) {
    if (!start || !count || index < 0 || static_cast<size_t>(index) >= part_count) return 0;
    if (static_cast<size_t>(s) + c > point_count) return 0;
    *start = s;
    *count = c;
    return 1;
}
} // namespace

extern "C" {
int hcp_planet(int index, int64_t unix_s, double lat, double lon, HcpPlanet* out) {
    hamclock::Planet p;
    if (!out || !planet_index(index, &p)) return 0;
    try {
        const hamclock::UtcSeconds t{unix_s};
        const auto h = hamclock::planet_horizontal(p, t, lat, lon);
        const auto pos = hamclock::planet_position(p, t);
        const auto sub = hamclock::planet_subpoint(p, t);
        *out = HcpPlanet{};
        copy(hamclock::planet_name(p), out->name, sizeof out->name);
        out->az_deg = h.azimuth_deg;
        out->el_deg = h.elevation_deg;
        out->sub_lat = sub.latitude_deg;
        out->sub_lon = sub.longitude_deg;
        out->distance_au = pos.distance_au;
        out->elongation_deg = pos.elongation_deg;
        out->ra_deg = pos.ra_deg;
        out->dec_deg = pos.dec_deg;
    } catch (const std::invalid_argument&) { return 0; }
    return 1;
}

int hcp_next_planet_event(int index, int64_t unix_s, int kind, double lat, double lon, int64_t* event_s) {
    hamclock::Planet p;
    if (!event_s || (kind != 0 && kind != 1) || !planet_index(index, &p)) return 0;
    const bool rise = kind == 0;
    try {
        int64_t lo = unix_s;
        bool was = after_event(rise, elevation(p, lo, lat, lon));
        for (int64_t hi = unix_s + kPlanetStepS; hi <= unix_s + kPlanetWindowS; lo = hi, hi += kPlanetStepS) {
            const bool now = after_event(rise, elevation(p, hi, lat, lon));
            if (was || !now) { was = now; continue; }
            while (hi - lo > 1) { // invariant: event state false at lo, true at hi
                const int64_t mid = lo + (hi - lo) / 2;
                (after_event(rise, elevation(p, mid, lat, lon)) ? hi : lo) = mid;
            }
            *event_s = hi;
            return 1;
        }
    } catch (const std::invalid_argument&) {}
    return 0;
}

int hcp_next_solar_eclipse(int64_t after_s, double lat, double lon, HcpEclipse* out) {
    if (!out) return 0;
    *out = HcpEclipse{};
    const auto e = hamclock::next_solar_eclipse(hamclock::UtcSeconds{after_s});
    if (!e) return 0;
    out->greatest_s = e->greatest.value;
    copy(hamclock::solar_eclipse_type_name(e->type), out->type, sizeof out->type);
    out->gamma = e->gamma;
    out->magnitude = e->magnitude;
    if (std::isfinite(lat) && std::isfinite(lon)) {
        try {
            const auto local = hamclock::eclipse_visibility(*e, lat, lon);
            out->visibility = static_cast<int>(local.visibility); // None 0, Marginal 1, Visible 2
            out->local_magnitude = local.magnitude;
            out->local_max_s = local.max_time.value;
            out->local_start_s = local.start_time.value;
            out->local_end_s = local.end_time.value;
        } catch (const std::invalid_argument&) {
            *out = HcpEclipse{};
            return 0;
        }
    }
    out->valid = 1;
    return 1;
}

int hcp_path(double de_lat, double de_lon, double dx_lat, double dx_lon, HcpPath* out) {
    if (!out) return 0;
    try {
        const auto g = hamclock::geometry({de_lat, de_lon}, {dx_lat, dx_lon});
        *out = HcpPath{g.status == hamclock::BearingStatus::Defined ? 1 : 0, g.distance_km, g.short_bearing_deg,
                       g.long_distance_km, g.long_bearing_deg};
    } catch (const std::invalid_argument&) { return 0; }
    return 1;
}

int hcp_maidenhead(double lat, double lon, int precision, char* buf, size_t cap) {
    if (precision != 2 && precision != 4 && precision != 6) return 0;
    try {
        return copy(hamclock::maidenhead({lat, lon}, static_cast<unsigned>(precision)), buf, cap);
    } catch (const std::invalid_argument&) { return 0; }
}

int hcp_land_ring_count(void) { return static_cast<int>(kLandRingCount); }

int hcp_land_ring(int index, uint32_t* start, uint32_t* count) {
    if (index < 0 || static_cast<size_t>(index) >= kLandRingCount) return 0;
    const auto& r = hamclock::land_rings[index];
    return part(index, kLandRingCount, kLandPointCount, r.start, r.count, start, count);
}

const float* hcp_land_points(size_t* point_count) {
    static const std::vector<float> points = [] {
        std::vector<float> v;
        v.reserve(2 * kLandPointCount);
        for (const auto& p : hamclock::land_points) {
            v.push_back(static_cast<float>(p.longitude));
            v.push_back(static_cast<float>(p.latitude));
        }
        return v;
    }();
    if (point_count) *point_count = kLandPointCount;
    return points.data();
}

int hcp_border_part_count(void) { return static_cast<int>(hamclock::border_part_count); }

int hcp_border_part(int index, uint32_t* start, uint32_t* count) {
    if (index < 0 || static_cast<size_t>(index) >= hamclock::border_part_count) return 0;
    const auto& b = hamclock::border_parts[index];
    return part(index, hamclock::border_part_count, hamclock::border_point_count, b.start, b.count, start, count);
}

const float* hcp_border_points(size_t* point_count) {
    static_assert(sizeof(hamclock::BorderPoint) == 2 * sizeof(float), "BorderPoint must be two packed floats");
    if (point_count) *point_count = hamclock::border_point_count;
    return &hamclock::border_points[0].longitude;
}
}
