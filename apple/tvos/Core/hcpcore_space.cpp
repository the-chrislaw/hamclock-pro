// C ABI: space-weather history, NOAA scales, OVATION aurora and D-RAP (see hcpcore.h).
#include "hcpcore.h"
#include "abi_util.hpp"
#include "aurora.hpp"
#include "drap.hpp"
#include "dst.hpp"
#include "scales.hpp"
#include "solarwind.hpp"
#include "swpc.hpp"
#include <algorithm>

using hcp::abi::copy;
using hcp::abi::view;

namespace {
// Writes the most recent min(size, cap) readings, still oldest first.
template <class Item, class Convert>
int fill_tail(const std::vector<Item>& items, HcpReading* out, size_t cap, Convert convert) {
    const size_t n = out ? std::min(items.size(), cap) : 0;
    const size_t first = items.size() - n;
    for (size_t i = 0; i < n; ++i) out[i] = convert(items[first + i]);
    return static_cast<int>(n);
}

HcpReading reading(const hcp::swpc::Reading& r) { return HcpReading{1, r.value, r.time_ms}; }
HcpReading reading(const hcp::solarwind::Point& p) { return HcpReading{1, p.value, p.time_ms}; }

// hcp_solarwind_series field number -> JSON key; nullptr when out of range.
const char* solarwind_field(int field) {
    static const char* const fields[] = {hcp::solarwind::kSpeed, hcp::solarwind::kDensity, hcp::solarwind::kBt,
                                         hcp::solarwind::kBz};
    return field >= 0 && field <= 3 ? fields[field] : nullptr;
}

HcpScaleDay day(const hcp::scales::Day& d) { return HcpScaleDay{d.r, d.s, d.g, d.r_minor, d.r_major, d.s_prob}; }
} // namespace

extern "C" {
int hcp_swpc_kp_series(const char* b, size_t n, HcpReading* out, size_t cap) {
    const auto s = hcp::swpc::kp_history(view(b, n));
    return s ? fill_tail(*s, out, cap, [](const auto& r) { return reading(r); }) : -1;
}

int hcp_swpc_xray_series(const char* b, size_t n, HcpReading* out, size_t cap) {
    const auto s = hcp::swpc::xray_history(view(b, n));
    return s ? fill_tail(*s, out, cap, [](const auto& r) { return reading(r); }) : -1;
}

int hcp_solarwind_series(const char* b, size_t n, int field, HcpReading* out, size_t cap) {
    if (!solarwind_field(field)) return -1;
    const auto s = hcp::solarwind::checked_series(view(b, n), solarwind_field(field));
    return s ? fill_tail(*s, out, cap, [](const auto& p) { return reading(p); }) : -1;
}

int hcp_solarwind_series_pair(const char* b, size_t n, int field_a, int field_b, HcpReading* out_a,
                              HcpReading* out_b, size_t cap, int* count_b) {
    if (count_b) *count_b = -1;
    if (!solarwind_field(field_a) || !solarwind_field(field_b)) return -1;
    const auto s = hcp::solarwind::checked_series(
        view(b, n), std::vector<std::string>{solarwind_field(field_a), solarwind_field(field_b)});
    if (!s) return -1;
    const auto convert = [](const auto& p) { return reading(p); };
    if (count_b) *count_b = fill_tail((*s)[1], out_b, cap, convert);
    return fill_tail((*s)[0], out_a, cap, convert);
}

int hcp_scales_parse(const char* b, size_t n, HcpScales* out) {
    if (!out) return 0;
    const auto s = hcp::scales::parse(view(b, n));
    *out = HcpScales{};
    out->valid = s.valid ? 1 : 0;
    out->observed_ms = s.observed_ms;
    out->current = day(s.current);
    out->past24h = day(s.past24h);
    for (size_t i = 0; i < 3; ++i) out->forecast[i] = day(s.forecast[i]);
    return out->valid;
}

int hcp_dst_parse(const char* b, size_t n, HcpDst* out, size_t cap) {
    const auto s = b ? hcp::dst::parse(view(b, n)) : std::nullopt;
    if (!s) return -1;
    const size_t count = out ? std::min(s->size(), cap) : 0;
    const size_t first = s->size() - count;
    for (size_t i = 0; i < count; ++i) out[i] = HcpDst{(*s)[first + i].time_ms, (*s)[first + i].dst};
    return static_cast<int>(count);
}

int hcp_dst_word(int dst, char* buf, size_t cap) { return copy(hcp::dst::word(dst), buf, cap); }

int hcp_scale_word(int level, char* buf, size_t cap) { return copy(hcp::scales::scale_word(level), buf, cap); }

int hcp_aurora_parse(const char* b, size_t n, HcpAurora* out, unsigned char* grid) {
    if (!out) return 0;
    const auto g = hcp::aurora::parse(view(b, n));
    *out = HcpAurora{g.valid ? 1 : 0, g.observed_ms, g.forecast_ms, hcp::aurora::peak(g, true),
                     hcp::aurora::peak(g, false)};
    if (grid) {
        constexpr size_t kCells = static_cast<size_t>(hcp::aurora::kLons * hcp::aurora::kLats);
        if (g.cells.size() == kCells) std::copy(g.cells.begin(), g.cells.end(), grid);
        else std::fill(grid, grid + kCells, static_cast<unsigned char>(0));
    }
    return out->valid;
}

int hcp_drap_parse(const char* b, size_t n, HcpDrap* out, float* grid) {
    if (!out) return 0;
    const auto g = hcp::drap::parse(view(b, n));
    *out = HcpDrap{};
    out->valid = g.valid ? 1 : 0;
    out->valid_ms = g.valid_ms;
    out->peak_mhz = hcp::drap::peak(g);
    copy(g.xray, out->xray, sizeof out->xray);
    copy(g.proton, out->proton, sizeof out->proton);
    if (grid) {
        constexpr size_t kCells = static_cast<size_t>(hcp::drap::kRows * hcp::drap::kCols);
        if (g.mhz.size() == kCells) std::copy(g.mhz.begin(), g.mhz.end(), grid);
        else std::fill(grid, grid + kCells, 0.0f);
    }
    return out->valid;
}
}
