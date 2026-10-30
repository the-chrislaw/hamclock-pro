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
/* Every TLE set in text, parsed ONCE and propagated to unix_s for the observer (the per-index calls
 * above re-parse the whole text each time). Fills out[] in file order with up to cap entries; valid
 * is 0 where SGP4 fails. Returns the number of sets (may exceed cap), 0 on bad input. */
typedef struct HcpSatInfo {
    char name[32];
    int valid;
    HcpSatState state;
} HcpSatInfo;
int hcp_sat_states(const char* text, size_t len, int64_t unix_s, double obs_lat_deg, double obs_lon_deg,
                   HcpSatInfo* out, size_t cap);

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

/* Kp / X-ray / solar-wind history for sparklines and bars. Fill out[] with up to cap readings,
 * OLDEST FIRST (most recent last), only valid rows; return count, or -1 when the body is not the product.
 * When there are more than cap rows the most recent cap are kept. */
int hcp_swpc_kp_series(const char* body, size_t len, HcpReading* out, size_t cap);      /* noaa-planetary-k-index.json */
int hcp_swpc_xray_series(const char* body, size_t len, HcpReading* out, size_t cap);    /* xrays-6-hour.json, 0.1-0.8 nm long channel only */
int hcp_solarwind_series(const char* body, size_t len, int field, HcpReading* out, size_t cap); /* field 0 speed (rtsw_wind_1m), 1 density (wind), 2 bt (rtsw_mag_1m), 3 bz (mag); active spacecraft rows as the latest_* functions choose */
/* Two fields of the same rtsw body from ONE parse (0+1 for wind, 2+3 for mag), same rows and order as
 * two hcp_solarwind_series calls. Returns field_a's count and writes field_b's to *count_b; -1 (and
 * *count_b = -1) when the body is not the product or a field is out of range. */
int hcp_solarwind_series_pair(const char* body, size_t len, int field_a, int field_b, HcpReading* out_a,
                              HcpReading* out_b, size_t cap, int* count_b);

/* NOAA scales (products/noaa-scales.json), port of src/widgets/solarscales/solar_scales_parse. -1 = not stated. */
typedef struct { int r, s, g; int r_minor, r_major, s_prob; } HcpScaleDay;
typedef struct {
    int valid;
    int64_t observed_ms;
    HcpScaleDay current, past24h;
    HcpScaleDay forecast[3];  /* today, tomorrow, day after */
} HcpScales;
int hcp_scales_parse(const char* body, size_t len, HcpScales* out);   /* returns out->valid */
/* Kyoto Dst (products/kyoto-dst.json), port of src/widgets/solardst/solar_dst_parse. Fills out[] with
 * the most recent cap hourly records, OLDEST FIRST; returns the count, or -1 when the body is not the product. */
typedef struct {
    int64_t time_ms; /* UTC, start of the hour */
    int dst;         /* nT */
} HcpDst;
int hcp_dst_parse(const char* body, size_t len, HcpDst* out, size_t cap);
int hcp_dst_word(int dst, char* buf, size_t cap); /* Quiet / Weak / Moderate / Intense / Super */
/* NOAA SWPC alerts, watches and warnings (products/alerts.json), port of
 * src/widgets/solaralerts/solar_alerts_parse. Texts are UTF-8, truncated to fit. */
typedef struct {
    char code[16];      /* SWPC product id, e.g. "K05A" */
    char kind[24];      /* Alert, Warning, Extended warning, Watch, Summary, Cancel */
    char headline[160];
    char scale[4];      /* "G1", "S2", "R3"...; empty when none stated */
    int64_t issued_ms;  /* UTC */
    int64_t until_ms;   /* UTC end of validity; 0 when the message has none */
    int cancelled;      /* 1 for a cancellation notice */
    int level;          /* 0 info, 1 minor/moderate or a watch/warning, 2 strong+ (scale >= 3) */
} HcpAlert;
/* Every parseable message, NEWEST FIRST, up to cap; returns the count or -1 when the body is not the product. */
int hcp_alerts_parse(const char* body, size_t len, HcpAlert* out, size_t cap);
/* The messages in force at now_ms out of a parsed list (newest per product, no cancellations, warnings
 * until their end time, alerts/summaries 24 h, watches 48 h). Returns the count written to out. */
int hcp_alerts_active(const HcpAlert* all, size_t count, int64_t now_ms, HcpAlert* out, size_t cap);
int hcp_scale_word(int level, char* buf, size_t cap);                  /* "Minor".."Extreme"; 0 and empty for 0/out of range */

/* OVATION aurora nowcast (json/ovation_aurora_latest.json), port of solar_aurora_parse.
 * grid may be NULL; otherwise it must hold 360*181 bytes, index (lat+90)*360 + lon(0..359), probability %. */
typedef struct { int valid; int64_t observed_ms, forecast_ms; int north_peak, south_peak; } HcpAurora;
int hcp_aurora_parse(const char* body, size_t len, HcpAurora* out, unsigned char* grid);  /* returns out->valid */

/* D-RAP (text/drap_global_frequencies.txt), port of solar_drap_parse.
 * grid may be NULL; otherwise 90*90 floats, row 0 = lat 89, step 2 down; col 0 = lon -178, step 4. */
typedef struct { int valid; int64_t valid_ms; double peak_mhz; char xray[64]; char proton[64]; } HcpDrap;
int hcp_drap_parse(const char* body, size_t len, HcpDrap* out, float* grid);  /* returns out->valid */

/* Launch Library 2 upcoming list, port of launches_parse (parse + ham_hint + pad_position). Soonest first.
 * Returns count or -1 when there is no "results" array. */
typedef struct {
    char name[96]; char provider[48]; char pad[64]; char location[96];
    char status[16]; char mission[96]; char mission_type[48]; char hint[64];
    int64_t net_ms; int precise; int has_pad; double pad_lat, pad_lon;
} HcpLaunch;
int hcp_launches_parse(const char* body, size_t len, HcpLaunch* out, size_t cap);
int hcp_launch_countdown(int64_t net_ms, int64_t now_ms, char* buf, size_t cap);  /* same text as countdown_text */

/* On-the-air spots, port of ota_parse (POTA api.pota.app/spot/activator, SOTA api-db2.sota.org.uk, WWFF spots.wwff.co).
 * Newest first, same skip/QRT rules as the Qt parse_* functions (which do not collapse duplicates; the Qt
 * merge() does that across feeds). Returns count or -1 when not a JSON array (as the Qt parser's ok=false). */
typedef struct {
    char program[8]; char call[16]; char reference[24]; char name[64];
    double freq_khz; char mode[12]; int64_t time_ms;
    int has_position; double lat, lon; char comment[64];
} HcpOtaSpot;
int hcp_pota_parse(const char* body, size_t len, HcpOtaSpot* out, size_t cap);
int hcp_sota_parse(const char* body, size_t len, HcpOtaSpot* out, size_t cap);
int hcp_wwff_parse(const char* body, size_t len, HcpOtaSpot* out, size_t cap);
int hcp_band_name(double freq_khz, char* buf, size_t cap);  /* "20m" etc, same band table the Qt dxcluster/ota filter uses; 0 when outside bands */

/* NASA FIRMS VIIRS CSV, port of src/widgets/fires/fires_parse. Strongest (FRP) first, as Qt does.
 * Returns count or -1 when not the expected CSV header. */
typedef struct { double lat, lon, frp_mw; int64_t time_ms; int high_confidence; } HcpFire;
int hcp_fires_parse(const char* body, size_t len, HcpFire* out, size_t cap);

/* Planets (src/domain/astro_planets): index 0..6 Mercury..Neptune. Returns 0 on bad input. */
typedef struct { char name[12]; double az_deg, el_deg, sub_lat, sub_lon, distance_au, elongation_deg, ra_deg, dec_deg; } HcpPlanet;
int hcp_planet(int index, int64_t unix_s, double lat_deg, double lon_deg, HcpPlanet* out);
/* Next rise (kind 0) / set (kind 1) of a planet strictly after unix_s: 5-minute elevation samples, then
 * bisection to 1 s; horizon 0 deg (geocentric, no refraction), 48 h search. 0 when none in the window. */
int hcp_next_planet_event(int index, int64_t unix_s, int kind, double lat_deg, double lon_deg, int64_t* event_s);

/* Solar eclipse (src/domain/astro_eclipse: next_solar_eclipse + eclipse_visibility). */
typedef struct {
    int valid; int64_t greatest_s; char type[12]; double gamma, magnitude;
    int visibility;            /* 0 none, 1 marginal, 2 visible at the station */
    double local_magnitude; int64_t local_max_s, local_start_s, local_end_s;
} HcpEclipse;
int hcp_next_solar_eclipse(int64_t after_s, double lat_deg, double lon_deg, HcpEclipse* out);  /* returns out->valid; lat/lon NaN = no station -> visibility 0 */

/* Great-circle path and locator (src/domain/geometry). Bearings are NaN when defined = 0. */
typedef struct { int defined; double distance_km, short_bearing_deg, long_distance_km, long_bearing_deg; } HcpPath;
int hcp_path(double de_lat, double de_lon, double dx_lat, double dx_lon, HcpPath* out);   /* 0 on invalid coords; defined=0 for coincident/antipodal */
int hcp_maidenhead(double lat_deg, double lon_deg, int precision, char* buf, size_t cap); /* 0 on invalid */

/* Map tables (src/map/land.hpp and src/map/borders.hpp), same style as hcp_coast_*: interleaved (lon,lat) floats. */
int hcp_land_ring_count(void);
int hcp_land_ring(int index, uint32_t* start, uint32_t* count);
const float* hcp_land_points(size_t* point_count);   /* converted once to a static float array */
int hcp_border_part_count(void);
int hcp_border_part(int index, uint32_t* start, uint32_t* count);
const float* hcp_border_points(size_t* point_count);

/* Place search for first-run setup. */
typedef struct { char name[160]; double lat, lon; } HcpPlace;
int hcp_nominatim_parse(const char* body, size_t len, HcpPlace* out, size_t cap); /* jsonv2 array as src/ui/location_tools.cpp parseSearchReply; -1 not an array */
int hcp_open_meteo_timezone(const char* body, size_t len, char* buf, size_t cap); /* "timezone" field of an Open-Meteo forecast reply; 0 when absent */

#ifdef __cplusplus
}
#endif
#endif
