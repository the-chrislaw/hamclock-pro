/* C ABI over the Qt-free HamClock Pro core, for the tvOS Swift front end.
 * Bytes in, plain structs out. No network, no clock, no allocation by callers. */
#ifndef HCPCORE_H
#define HCPCORE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int valid;          /* 0 when the body could not be parsed */
    double value;
    int64_t time_ms;    /* UTC observation time from the product */
} HcpReading;

HcpReading hcp_swpc_latest_kp(const char* body, size_t len);
HcpReading hcp_swpc_latest_solar_flux(const char* body, size_t len);
HcpReading hcp_swpc_latest_xray(const char* body, size_t len);

/* NOAA rtsw_*_1m.json: latest active-spacecraft value (see solarwind.hpp). */
HcpReading hcp_solarwind_latest_speed(const char* body, size_t len);   /* km/s */
HcpReading hcp_solarwind_latest_density(const char* body, size_t len); /* p/cm^3 */
HcpReading hcp_solarwind_latest_bt(const char* body, size_t len);      /* nT */
HcpReading hcp_solarwind_latest_bz(const char* body, size_t len);      /* nT, GSM */
int hcp_solarwind_speed_description(double km_s, char* buf, size_t cap);
int hcp_solarwind_bz_description(double nt, char* buf, size_t cap);

/* USGS GeoJSON summary feed (see quakes.hpp). */
typedef struct {
    double magnitude;
    double latitude, longitude, depth_km;
    int64_t time_ms;    /* origin time, UTC */
    char place[96];     /* NUL-terminated, truncated */
} HcpQuake;
/* Fills out[] with up to cap quakes, newest first. Returns the number written,
 * or -1 when the body is not a GeoJSON FeatureCollection. */
int hcp_quakes_parse(const char* body, size_t len, HcpQuake* out, size_t cap);
/* Index (into the full parsed list) of the strongest quake, or -1. */
int hcp_quakes_strongest_index(const char* body, size_t len);
int hcp_quake_magnitude_class(double magnitude, char* buf, size_t cap);

/* Open-Meteo `current` forecast reply (see weather.hpp). Returns 1 and fills *out, or 0
 * when the body is rejected (missing or out-of-range fields). */
typedef struct {
    double temperature_c, wind_kmh, dew_point_c, pressure_hpa, pressure_change_hpa;
    int humidity_pct, wind_dir_deg, weather_code, is_day;
    int has_dew_point, has_pressure, has_pressure_change;
    char description[32]; /* WMO text, e.g. "Light rain" */
    char wind_from[4];    /* 16-point compass, direction the wind comes from */
    char trend[8];        /* Rising / Falling / Steady, empty without a pressure change */
} HcpWeather;
int hcp_weather_parse(const char* body, size_t len, HcpWeather* out);

/* NHC CurrentStorms.json active-storm index (see storms.hpp). Strongest first; returns the
 * number written (0 = no active storms), or -1 when the body is not an NHC index. */
typedef struct {
    char name[24];
    char classification[24]; /* "Hurricane", "Tropical storm", ... */
    char heading[4];         /* compass point of movement, empty when unknown */
    int wind_kt, pressure_mb, category, speed_kt; /* pressure 0 / speed -1 when not given; category 0 below hurricane */
    double latitude, longitude;
    int64_t issued_ms;       /* advisory issuance, UTC; 0 when unknown */
} HcpStorm;
int hcp_storms_parse(const char* body, size_t len, HcpStorm* out, size_t cap);

/* Writes a NUL-terminated string into buf (truncated to cap); returns 1 when non-empty. */
int hcp_swpc_xray_class(double flux_w_m2, char* buf, size_t cap);
int hcp_swpc_kp_description(double kp, char* buf, size_t cap);

/* Offline astronomy (shared solar_position code); computed from the supplied UTC
 * seconds, so it is exact for that instant and never "stale". Returns 0 on bad input. */
int hcp_subsolar_point(int64_t unix_s, double* lat_deg, double* lon_deg);
int hcp_sun_elevation(int64_t unix_s, double lat_deg, double lon_deg, double* elevation_deg);

/* Next sunrise (kind 0) or sunset (kind 1) strictly after unix_s, UTC seconds.
 * Returns 0 for bad input or no event within the search window (polar regions). */
int hcp_next_sun_event(int64_t unix_s, int kind, double lat_deg, double lon_deg, int64_t* event_s);
/* Same for the Moon (upper limb). */
int hcp_next_moon_event(int64_t unix_s, int kind, double lat_deg, double lon_deg, int64_t* event_s);
/* Moon phase name (written to buf) and illuminated fraction 0..1. */
int hcp_moon_phase(int64_t unix_s, char* buf, size_t cap, double* illuminated);

/* Centre of a 2/4/6-character Maidenhead locator (shared locator_input decoder).
 * Returns 0 for anything but a valid locator. */
int hcp_locator_center(const char* locator, double* lat_deg, double* lon_deg);

/* Satellites (shared Qt-free TLE parser + vendored SGP4). Computed on device from the
 * supplied TLE; the caller owns TLE freshness (epoch age) and must label stale elements. */
typedef struct HcpSatState {
    double lat_deg, lon_deg, alt_km;       /* sub-satellite point */
    double az_deg, el_deg, range_km;       /* look angle from the observer */
    double range_rate_km_s;                /* > 0: receding */
    int sunlit;                            /* 1 sunlit, 0 eclipsed */
    int64_t epoch_s;                       /* TLE epoch, unix seconds */
} HcpSatState;
/* Number of valid TLE sets in text (3-line or bare 2-line). Names are copied to name_out
 * (cap each) when non-null, up to max_names entries laid out contiguously. */
int hcp_tle_count(const char* text, size_t len);
/* State of the index-th TLE in text at unix_s for the observer. Returns 0 on bad input,
 * an out-of-range index or an SGP4 failure (e.g. decayed). */
int hcp_sat_state(const char* text, size_t len, int index, int64_t unix_s,
                  double obs_lat_deg, double obs_lon_deg, HcpSatState* out);
/* Name of the index-th TLE. */
int hcp_tle_name(const char* text, size_t len, int index, char* buf, size_t cap);

/* Next passes of the index-th TLE for the observer (iPhone widgets). Passes reach at least
 * min_el_deg; a satellite already up gives a pass with aos_s == from_s. Searches horizon_s
 * seconds ahead. Returns the number written, or -1 on bad input (null text/out, bad
 * coordinates, index out of range, invalid elements). Azimuths are degrees from true north. */
typedef struct {
    int64_t aos_s, tca_s, los_s;
    double max_el_deg, aos_az_deg, los_az_deg;
} HcpPass;
int hcp_sat_passes(const char* text, size_t len, int index, int64_t from_s, int64_t horizon_s,
                   double obs_lat_deg, double obs_lon_deg, double min_el_deg, HcpPass* out, size_t cap);
/* Short display name of the index-th TLE, the form the Qt app saves in [satellites] tracked=
 * ("ISS (ZARYA)" -> "ISS"). */
int hcp_tle_short_name(const char* text, size_t len, int index, char* buf, size_t cap);
/* Index of the first TLE whose short name equals short_name (ASCII case-insensitive), or -1. */
int hcp_tle_find_short_name(const char* text, size_t len, const char* short_name);

/* HF band outlook (shared band_outlook heuristic; NOT VOACAP). Conditions: 0 Poor, 1 Fair, 2 Good.
 * Pass a_index = 0 when the A index is unknown (only the Kp storm rule then applies).
 * Fills out[] with up to cap bands (80m..10m); returns the number written. */
typedef struct {
    char band[8];
    double mhz;
    int day, night;
} HcpBand;
int hcp_hf_outlook(double sfi, double kp, double a_index, HcpBand* out, size_t cap);

/* World coastline from the shared Natural Earth table (map/coastline.hpp), as polylines.
 * Points are interleaved (lon, lat) floats in degrees; part i is points [start, start+count). */
int hcp_coast_part_count(void);
int hcp_coast_part(int index, uint32_t* start, uint32_t* count);
const float* hcp_coast_points(size_t* point_count);

#ifdef __cplusplus
}
#endif
#endif
