#include "hcpcore.h"
#include "solarwind.hpp"
#include "swpc.hpp"
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
}
