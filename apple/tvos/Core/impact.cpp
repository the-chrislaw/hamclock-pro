#include "impact.hpp"
#include <algorithm>

namespace hcp::impact {
namespace {
constexpr int64_t kHourMs = 3600 * 1000LL;
constexpr int64_t kDstFreshMs = 6 * kHourMs; // an old storm must not rank as current
constexpr int64_t kDayMs = 24 * kHourMs;

struct Row { char letter; int now; int max; };

std::vector<Row> ordered(const scales::Scales& d, const std::vector<dst::Sample>& dst, int64_t now_ms) {
    std::vector<Row> rows{{'R', d.current.r, d.past24h.r}, {'S', d.current.s, d.past24h.s}, {'G', d.current.g, d.past24h.g}};
    if (!dst.empty() && now_ms - dst.back().time_ms < kDstFreshMs) {
        int low = dst.back().dst;
        for (const auto& s : dst)
            if (s.time_ms >= dst.back().time_ms - kDayMs) low = std::min(low, s.dst);
        rows.push_back({'D', dst_level(dst.back().dst), dst_level(low)});
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        return a.now != b.now ? a.now > b.now : a.max > b.max;
    });
    return rows;
}

std::string word(const Row& row, const std::vector<dst::Sample>& dst) {
    return row.letter == 'D' ? dst::word(dst.back().dst) : scales::scale_word(row.now);
}
} // namespace

std::string effect(char letter, int level) {
    static const char* const r[] = {"No HF impact", "Weak HF fades on the sunlit side",
                                    "Limited HF blackout on the sunlit side", "Wide-area HF blackout, about an hour",
                                    "HF blackout on the sunlit side for 1-2 h", "Complete HF blackout on the sunlit side"};
    static const char* const s[] = {"No radiation impact", "Minor polar HF absorption", "Small effects on polar HF",
                                    "Degraded HF on polar paths", "Polar HF blackout likely", "Complete polar HF blackout"};
    static const char* const g[] = {"Quiet geomagnetic field", "Aurora at high latitudes",
                                    "Aurora to about 55 deg; HF fades at high latitudes",
                                    "Aurora to about 50 deg; intermittent HF and GPS problems",
                                    "Aurora to about 45 deg; HF and GPS degraded",
                                    "Aurora to low latitudes; HF may fail for days"};
    static const char* const d[] = {"Quiet ring current", "Weak storm", "Moderate storm; HF fades at high latitudes",
                                    "Intense storm; HF and GPS degraded", "", "Super storm; HF may fail for days"};
    if (level < 0 || level > 5) return {};
    switch (letter) {
    case 'R': return r[level];
    case 'S': return s[level];
    case 'G': return g[level];
    case 'D': return d[level];
    default: return {};
    }
}

int dst_level(int dst) { return dst > -30 ? 0 : dst > -50 ? 1 : dst > -100 ? 2 : dst > -250 ? 3 : 5; }

std::vector<Entry> ranked(const scales::Scales& s, const std::vector<dst::Sample>& dst, int64_t now_ms) {
    std::vector<Entry> out;
    if (!s.valid) return out;
    for (const Row& row : ordered(s, dst, now_ms))
        out.push_back({row.letter, row.now, word(row, dst), row.max, effect(row.letter, row.now)});
    return out;
}

std::string headline(const scales::Scales& s, const std::vector<dst::Sample>& dst, int64_t now_ms) {
    if (!s.valid) return {};
    const Row top = ordered(s, dst, now_ms).front();
    if (top.now <= 0) return "All quiet";
    return std::string(1, top.letter) + std::to_string(top.now) + " " + word(top, dst) + ": " + effect(top.letter, top.now);
}

} // namespace hcp::impact
