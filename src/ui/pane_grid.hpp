#pragma once

namespace hamclock {

// Sizes in logical pixels. min_map_share is the fraction of `height` the map
// must keep below the panes; max_panes is the number of saved pane slots.
struct PaneGridInput {
    double width;
    double height;
    double gap;
    double min_pane_width;
    double min_pane_height;
    double min_map_share;
    int max_panes;
};

struct PaneGrid {
    int columns;
    int rows;
    double height;   // height of the whole pane area; 0 for bad input (the caller keeps its own)
};

// How many pane columns and rows fit the pane area, and how tall that area is:
// one row takes 40 % of the height (the 1.0 layout), more rows take everything
// the map leaves. Never fewer than three columns; bad input returns 3 x 1.
PaneGrid fit_pane_grid(const PaneGridInput& in);

} // namespace hamclock
