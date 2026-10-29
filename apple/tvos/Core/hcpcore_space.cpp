// C ABI: space-weather history, NOAA scales, OVATION aurora and D-RAP (see hcpcore.h).
#include "hcpcore.h"
#include "abi_util.hpp"
#include "aurora.hpp"
#include "drap.hpp"
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
    static const char* const fields[] = {hcp::solarwind::kSpeed, hcp::solarwind::kDensity, hcp::solarwind::kBt,
                                         hcp::solarwind::kBz};
    if (field < 0 || field > 3) return -1;
    const auto s = hcp::solarwind::checked_series(view(b, n), fields[field]);
    return s ? fill_tail(*s, out, cap, [](const auto& p) { return reading(p); }) : -1;
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
