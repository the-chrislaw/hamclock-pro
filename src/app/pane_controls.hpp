#pragma once

#include <array>
#include <optional>

namespace hamclock {
struct PaneControls {
    std::array<unsigned, 3> selected{0, 1, 2};
    std::optional<unsigned> focused;

    bool focus_next(bool backwards = false);
    bool move_focus(int direction);
    bool activate();
    bool choose(unsigned index);
};
int run_pane_control_tests();
} // namespace hamclock
