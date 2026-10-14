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

/* Writes a NUL-terminated string into buf (truncated to cap); returns 1 when non-empty. */
int hcp_swpc_xray_class(double flux_w_m2, char* buf, size_t cap);
int hcp_swpc_kp_description(double kp, char* buf, size_t cap);

#ifdef __cplusplus
}
#endif
#endif
