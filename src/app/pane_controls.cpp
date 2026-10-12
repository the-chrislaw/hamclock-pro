#include "pane_controls.hpp"

namespace hamclock {
bool PaneControls::focus_next(bool backwards) {
    if (!focused || *focused >= selected.size())
        focused = backwards ? 2U : 0U;
    else
        focused = backwards ? (*focused + 2U) % 3U : (*focused + 1U) % 3U;
    return true;
}

bool PaneControls::move_focus(int direction) {
    if (direction != -1 && direction != 1) return false;
    return focus_next(direction == -1);
}

bool PaneControls::activate() {
    if (!focused || *focused >= selected.size()) return false;
    selected[*focused] = (selected[*focused] % 5U + 1U) % 5U;
    return true;
}

bool PaneControls::choose(unsigned index) {
    if (index >= selected.size()) return false;
    focused = index;
    return activate();
}
} // namespace hamclock
