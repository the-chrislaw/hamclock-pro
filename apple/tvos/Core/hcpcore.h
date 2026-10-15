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

/* Writes a NUL-terminated string into buf (truncated to cap); returns 1 when non-empty. */
int hcp_swpc_xray_class(double flux_w_m2, char* buf, size_t cap);
int hcp_swpc_kp_description(double kp, char* buf, size_t cap);

/* Offline astronomy (shared solar_position code); computed from the supplied UTC
 * seconds, so it is exact for that instant and never "stale". Returns 0 on bad input. */
int hcp_subsolar_point(int64_t unix_s, double* lat_deg, double* lon_deg);
int hcp_sun_elevation(int64_t unix_s, double lat_deg, double lon_deg, double* elevation_deg);

#ifdef __cplusplus
}
#endif
#endif
