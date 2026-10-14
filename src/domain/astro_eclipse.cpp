#include "astro_eclipse.hpp"
#include "astro_moon.hpp"
#include "solar_position.hpp"
#include <cmath>
#include <stdexcept>

namespace hamclock {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeltaT = 69.0; // TT - UTC, seconds (as in astro_moon.cpp)
double rad(double d) { return d * kPi / 180.0; }

// Eclipse circumstances of the new moon with index k, or empty when no eclipse.
std::optional<SolarEclipse> eclipse_for_k(double k) {
    const double t = k / 1236.85;
    const double t2 = t * t, t3 = t2 * t, t4 = t3 * t;
    const double jde0 = 2451550.09766 + 29.530588861 * k + 0.00015437 * t2 - 0.000000150 * t3 + 0.00000000073 * t4;
    const double e = 1 - 0.002516 * t - 0.0000074 * t2;
    const double m = rad(2.5534 + 29.10535670 * k - 0.0000014 * t2 - 0.00000011 * t3);
    const double mp = rad(201.5643 + 385.81693528 * k + 0.0107582 * t2 + 0.00001238 * t3 - 0.000000058 * t4);
    const double f = rad(160.7108 + 390.67050284 * k - 0.0016118 * t2 - 0.00000227 * t3 + 0.000000011 * t4);
    const double om = rad(124.7746 - 1.56375588 * k + 0.0020672 * t2 + 0.00000215 * t3);
    if (std::fabs(std::sin(f)) > 0.36) return std::nullopt;
    const double f1 = f - rad(0.02665) * std::sin(om);

    const double dt = -0.4075 * std::sin(mp) + 0.1721 * e * std::sin(m) + 0.0161 * std::sin(2 * mp)
        - 0.0097 * std::sin(2 * f1) + 0.0073 * e * std::sin(mp - m) - 0.0050 * e * std::sin(mp + m)
        - 0.0023 * std::sin(mp - 2 * f1) + 0.0021 * e * std::sin(2 * m) + 0.0012 * std::sin(mp + 2 * f1)
        + 0.0006 * e * std::sin(2 * mp + m) - 0.0004 * std::sin(3 * mp) - 0.0003 * e * std::sin(m + 2 * f1)
        + 0.0003 * std::sin(rad(299.77 + 0.107408 * k - 0.009173 * t2)) - 0.0002 * e * std::sin(m - 2 * f1)
        - 0.0002 * e * std::sin(2 * mp - m) - 0.0002 * std::sin(om);
    const double jde = jde0 + dt;

    const double p = 0.2070 * e * std::sin(m) + 0.0024 * e * std::sin(2 * m) - 0.0392 * std::sin(mp)
        + 0.0116 * std::sin(2 * mp) - 0.0073 * e * std::sin(mp + m) + 0.0067 * e * std::sin(mp - m)
        + 0.0118 * std::sin(2 * f1);
    const double q = 5.2207 - 0.0048 * e * std::cos(m) + 0.0020 * e * std::cos(2 * m) - 0.3299 * std::cos(mp)
        - 0.0060 * e * std::cos(mp + m) + 0.0041 * e * std::cos(mp - m);
    const double w = std::fabs(std::cos(f1));
    const double gamma = (p * std::cos(f1) + q * std::sin(f1)) * (1 - 0.0048 * w);
    const double u = 0.0059 + 0.0046 * e * std::cos(m) - 0.0182 * std::cos(mp) + 0.0004 * std::cos(2 * mp)
        - 0.0005 * std::cos(m + mp);

    const double ag = std::fabs(gamma);
    if (ag > 1.5433 + u) return std::nullopt;
    SolarEclipse out{};
    out.gamma = gamma;
    out.magnitude = 0;
    if (ag < 0.9972) {
        if (u < 0) out.type = SolarEclipseType::Total;
        else if (u > 0.0047) out.type = SolarEclipseType::Annular;
        else {
            const double omega = 0.00464 * std::sqrt(1 - gamma * gamma);
            out.type = u < omega ? SolarEclipseType::Hybrid : SolarEclipseType::Annular;
        }
    } else {
        out.type = SolarEclipseType::Partial; // includes non-central total/annular, shown as partial-grazing
        out.magnitude = (1.5433 + u - ag) / (0.5461 + 2 * u);
    }
    const double unix_tt = (jde - 2440587.5) * 86400.0;
    out.greatest = UtcSeconds{static_cast<std::int64_t>(std::llround(unix_tt - kDeltaT))};
    return out;
}
} // namespace

std::optional<SolarEclipse> next_solar_eclipse(UtcSeconds after) {
    const double years = (static_cast<double>(after.value) / 86400.0 + 2440587.5 - 2451545.0) / 365.25;
    double k = std::floor(years * 12.3685) - 1;
    for (int i = 0; i < 30 + 2; ++i, k += 1) {
        const auto ecl = eclipse_for_k(k);
        if (ecl && ecl->greatest.value > after.value) return ecl;
    }
    return std::nullopt;
}

const char* solar_eclipse_type_name(SolarEclipseType t) {
    switch (t) {
    case SolarEclipseType::Partial: return "Partial";
    case SolarEclipseType::Annular: return "Annular";
    case SolarEclipseType::Total: return "Total";
    case SolarEclipseType::Hybrid: return "Hybrid";
    }
    return "";
}

EclipseLocal eclipse_visibility(const SolarEclipse& e, double lat, double lon) {
    if (!std::isfinite(lat) || !std::isfinite(lon) || lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
        throw std::invalid_argument("invalid location");
    EclipseLocal out{EclipseVisibility::None, 180.0, 0.0};
    const double la = rad(lat);
    for (int s = -240; s <= 240; s += 2) {
        const UtcSeconds t{e.greatest.value + s * 60};
        const SubsolarPoint sub = subsolar_point(t);
        const double sun_el = solar_elevation_deg(sub, lat, lon);
        if (sun_el <= 0.0) continue;
        // Sun azimuth from north from its declination and hour angle (west positive).
        const double dec = rad(sub.latitude_deg), h = rad(lon - sub.longitude_deg);
        const double sun_az = std::atan2(std::sin(h), std::cos(h) * std::sin(la) - std::tan(dec) * std::cos(la)) + kPi;
        const MoonHorizontal m = moon_horizontal(t, lat, lon);
        const double se = rad(sun_el), me = rad(m.elevation_deg), da = sun_az - rad(m.azimuth_deg);
        double c = std::sin(se) * std::sin(me) + std::cos(se) * std::cos(me) * std::cos(da);
        c = std::fmax(-1.0, std::fmin(1.0, c));
        const double sep = std::acos(c) * 180.0 / kPi;
        if (sep < out.min_separation_deg) { out.min_separation_deg = sep; out.sun_elevation_deg = sun_el; }
    }
    if (out.min_separation_deg < 0.40) out.visibility = EclipseVisibility::Visible;
    else if (out.min_separation_deg < 0.64) out.visibility = EclipseVisibility::Marginal;
    return out;
}
}
