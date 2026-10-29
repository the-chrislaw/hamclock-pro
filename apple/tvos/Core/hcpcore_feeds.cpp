// C ABI: launches, on-the-air spots, FIRMS fires and place search (see hcpcore.h).
#include "hcpcore.h"
#include "abi_util.hpp"
#include "fires.hpp"
#include "launches.hpp"
#include "ota.hpp"
#include "places.hpp"
#include <algorithm>

using hcp::abi::copy;
using hcp::abi::view;

namespace {
HcpLaunch launch(const hcp::launches::Launch& l) {
    HcpLaunch o{};
    copy(l.name, o.name, sizeof o.name);
    copy(l.provider, o.provider, sizeof o.provider);
    copy(l.pad, o.pad, sizeof o.pad);
    copy(l.location, o.location, sizeof o.location);
    copy(l.status, o.status, sizeof o.status);
    copy(l.mission, o.mission, sizeof o.mission);
    copy(l.mission_type, o.mission_type, sizeof o.mission_type);
    copy(hcp::launches::ham_hint(l), o.hint, sizeof o.hint);
    o.net_ms = l.net_ms;
    o.precise = l.precise ? 1 : 0;
    if (const auto p = hcp::launches::pad_position(l)) {
        o.has_pad = 1;
        o.pad_lat = p->lat;
        o.pad_lon = p->lon;
    }
    return o;
}

HcpOtaSpot spot(const hcp::ota::Spot& s) {
    HcpOtaSpot o{};
    copy(s.program, o.program, sizeof o.program);
    copy(s.call, o.call, sizeof o.call);
    copy(s.reference, o.reference, sizeof o.reference);
    copy(s.name, o.name, sizeof o.name);
    o.freq_khz = s.freq_khz;
    copy(s.mode, o.mode, sizeof o.mode);
    o.time_ms = s.time_ms;
    o.has_position = s.has_position ? 1 : 0;
    o.lat = s.latitude;
    o.lon = s.longitude;
    copy(s.comment, o.comment, sizeof o.comment);
    return o;
}

HcpFire fire(const hcp::fires::Detection& d) {
    return HcpFire{d.latitude, d.longitude, d.frp_mw, d.acquired_ms,
                   d.confidence == hcp::fires::Detection::High ? 1 : 0};
}

HcpPlace place(const hcp::places::Place& p) {
    HcpPlace o{};
    copy(p.name, o.name, sizeof o.name);
    o.lat = p.lat;
    o.lon = p.lon;
    return o;
}

// First min(size, cap) items in list order; -1 when the parser rejected the body.
template <class List, class Out, class Convert>
int fill_head(const std::optional<List>& list, Out* out, size_t cap, Convert convert) {
    if (!list) return -1;
    const size_t n = out ? std::min(list->size(), cap) : 0;
    for (size_t i = 0; i < n; ++i) out[i] = convert((*list)[i]);
    return static_cast<int>(n);
}
} // namespace

extern "C" {
int hcp_launches_parse(const char* b, size_t n, HcpLaunch* out, size_t cap) {
    return fill_head(hcp::launches::parse(view(b, n)), out, cap, launch);
}

int hcp_launch_countdown(int64_t net_ms, int64_t now_ms, char* buf, size_t cap) {
    return copy(hcp::launches::countdown_text(net_ms, now_ms), buf, cap);
}

int hcp_pota_parse(const char* b, size_t n, HcpOtaSpot* out, size_t cap) {
    return fill_head(hcp::ota::parse_pota(view(b, n)), out, cap, spot);
}

int hcp_sota_parse(const char* b, size_t n, HcpOtaSpot* out, size_t cap) {
    return fill_head(hcp::ota::parse_sota(view(b, n)), out, cap, spot);
}

int hcp_wwff_parse(const char* b, size_t n, HcpOtaSpot* out, size_t cap) {
    return fill_head(hcp::ota::parse_wwff(view(b, n)), out, cap, spot);
}

int hcp_band_name(double freq_khz, char* buf, size_t cap) { return copy(hcp::ota::band_for_khz(freq_khz), buf, cap); }

int hcp_fires_parse(const char* b, size_t n, HcpFire* out, size_t cap) {
    return fill_head(hcp::fires::parse(view(b, n)), out, cap, fire);
}

int hcp_nominatim_parse(const char* b, size_t n, HcpPlace* out, size_t cap) {
    return fill_head(hcp::places::parse_nominatim(view(b, n)), out, cap, place);
}

int hcp_open_meteo_timezone(const char* b, size_t n, char* buf, size_t cap) {
    if (buf && cap) buf[0] = '\0';
    return copy(hcp::places::open_meteo_timezone(view(b, n)), buf, cap);
}
}
