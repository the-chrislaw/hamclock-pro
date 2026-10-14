#pragma once
#include "clock_state.hpp"
#include <optional>
#include <vector>
namespace hamclock {
// Offline sunrise/sunset (NOAA Solar Calculator method after Meeus, with the
// standard 90.833 degree zenith: refraction plus the solar semi-diameter).
// Accurate to about a minute outside the polar circles.
enum class SunEventKind { Rise, Set };
struct SunEvent { SunEventKind kind; UtcSeconds time; };
enum class PolarState { None, MidnightSun, PolarNight };

// Rise and set of the UTC day starting at day_start_utc (a multiple of 86400);
// only events with time in [day_start, day_start + 86400) are returned, sorted.
// polar says why a day without events has none (sun stays up or down).
// Latitude must be in [-90, 90], longitude (east positive) in [-180, 180];
// otherwise std::invalid_argument.
struct SunDay { std::vector<SunEvent> events; PolarState polar = PolarState::None; };
SunDay sun_events_for_utc_day(std::int64_t day_start_utc, double latitude_deg, double longitude_deg);

// First event of the given kind strictly after `after`, searching up to two
// days ahead; empty when none (polar regions).
std::optional<UtcSeconds> next_sun_event(UtcSeconds after, SunEventKind kind,
                                         double latitude_deg, double longitude_deg);

// Yearly sunrise/sunset ("grayline") series for the chart: one entry per UTC
// day of `year`. Times are minutes after 00:00 UTC of that day; -1 when the day
// has no such event (see polar). Same accuracy and argument checks as above.
struct SunYearDay { int day_of_year; double rise_min; double set_min; PolarState polar; };
std::vector<SunYearDay> sun_year_series(int year, double latitude_deg, double longitude_deg);
}
