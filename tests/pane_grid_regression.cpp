// Pane grid: as many panes as fit at their minimum size, never fewer than the
// classic three, a second row only when the map keeps its share of the height.
#include "../src/ui/pane_grid.hpp"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool is(hamclock::PaneGrid grid, int columns, int rows) { return grid.columns == columns && grid.rows == rows; }
hamclock::PaneGridInput input(double width, double height, double map_share) {
    return {width, height, 10, 260, 220, map_share, 12};
}
} // namespace

int main() {
    using hamclock::fit_pane_grid;

    // 7-inch panel (dense tokens): three panes, one row, as before.
    check(is(fit_pane_grid({845, 590, 5, 250, 190, 0.55, 12}), 3, 1), "7-inch keeps three panes");
    check(fit_pane_grid({845, 590, 5, 250, 190, 0.55, 12}).height == 236, "one row takes 40 % of the height");
    // ...and two rows of three when the owner picks a small map.
    check(is(fit_pane_grid({845, 590, 5, 250, 190, 0.28, 12}), 3, 2), "7-inch small map shows six");
    check(fit_pane_grid({845, 590, 5, 250, 190, 0.28, 12}).height >= 2 * 190 + 5, "two rows are at least the minimum height");
    // Narrow areas still get three (the panes clip rather than vanish).
    check(is(fit_pane_grid({600, 400, 5, 250, 190, 0.55, 12}), 3, 1), "never fewer than three");
    // A laptop window: a fourth pane fits in the row; two rows would squeeze the map.
    check(is(fit_pane_grid(input(1180, 840, 0.55)), 4, 1), "laptop gets four");
    // A big monitor: five across and a second row while the map keeps 55 %.
    check(is(fit_pane_grid(input(1500, 1100, 0.55)), 5, 2), "monitor gets two rows of five");
    // ...and those rows share all the height the map leaves, not just the minimum.
    check(std::abs(fit_pane_grid(input(1500, 1100, 0.55)).height - (0.45 * 1100 - 10)) < 1e-6, "two rows fill what the map leaves");
    // Wider still: the slot limit caps the grid, dropping columns first.
    check(is(fit_pane_grid(input(2000, 1100, 0.55)), 6, 2), "capped at twelve panes");
    // A slot limit below three columns' worth of a second row keeps one row.
    check(is(fit_pane_grid({1500, 1100, 10, 260, 220, 0.55, 5}), 5, 1), "small cap keeps one row");

    // iPad portrait: a tall, narrow area gets two wider columns and a second row.
    check(is(fit_pane_grid(input(600, 1100, 0.55)), 2, 2), "tall narrow area gets two columns");
    // ...while a wide area just as narrow (Pi 800x480 pane area) keeps three.
    check(is(fit_pane_grid({600, 450, 5, 250, 190, 0.55, 12}), 3, 1), "wide narrow area keeps three");

    // Garbage in, the classic layout out.
    const double nan = std::numeric_limits<double>::quiet_NaN();
    check(is(fit_pane_grid({nan, 600, 5, 250, 190, 0.55, 12}), 3, 1) && fit_pane_grid({nan, 600, 5, 250, 190, 0.55, 12}).height == 0, "NaN width");
    check(is(fit_pane_grid({1000, 600, 5, 0, 190, 0.55, 12}), 3, 1), "zero pane width");
    check(is(fit_pane_grid({1000, 600, 5, 250, -1, 0.55, 12}), 3, 1), "negative pane height");
    check(is(fit_pane_grid({1000, 600, 5, 250, 190, 0.55, 0}), 3, 1), "zero cap");

    std::cout << (failures ? "FAILED\n" : "pane-grid-regression: ok\n");
    return failures ? 1 : 0;
}
