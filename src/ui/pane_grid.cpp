#include "pane_grid.hpp"

#include <algorithm>
#include <cmath>

namespace hamclock {
namespace {
constexpr int kMinColumns = 3;
constexpr int kMaxRows = 2;
constexpr double kSingleRowShare = 0.4;
constexpr PaneGrid kClassic{kMinColumns, 1, 0};

bool positive(double value) { return std::isfinite(value) && value > 0; }
} // namespace

PaneGrid fit_pane_grid(const PaneGridInput& in) {
    if (!positive(in.width) || !positive(in.height) || !positive(in.min_pane_width) ||
        !positive(in.min_pane_height) || !std::isfinite(in.gap) || in.gap < 0 ||
        !std::isfinite(in.min_map_share) || in.max_panes < 1)
        return kClassic;

    const int fit = static_cast<int>(std::floor((in.width + in.gap) / (in.min_pane_width + in.gap)));
    const int columns = std::max(kMinColumns, std::min(fit, in.max_panes));

    // Add a row only while the map below keeps its share and the slots last.
    int rows = 1;
    for (int candidate = 2; candidate <= kMaxRows; ++candidate) {
        const double panes_height = candidate * in.min_pane_height + (candidate - 1) * in.gap;
        const double map_height = in.height - panes_height - in.gap;
        if (map_height < in.min_map_share * in.height) break;
        if (std::min(columns, in.max_panes / candidate) < kMinColumns) break;
        rows = candidate;
    }
    const double height = rows == 1
        ? std::max(in.min_pane_height, kSingleRowShare * in.height)
        : std::max(rows * in.min_pane_height + (rows - 1) * in.gap, (1 - in.min_map_share) * in.height - in.gap);
    return {std::min(columns, in.max_panes / rows), rows, height};
}

} // namespace hamclock
