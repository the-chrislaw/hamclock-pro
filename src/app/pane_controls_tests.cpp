#include "pane_controls.hpp"

#include <iostream>
#include <limits>

namespace hamclock {
int run_pane_control_tests() {
    int failures = 0;
    auto check = [&](bool condition, const char* name) {
        if (!condition) { ++failures; std::cerr << "Pane control test failed: " << name << '\n'; }
    };
    PaneControls pane;
    check(!pane.focused && pane.selected == std::array<unsigned, 3>{0, 1, 2}, "initial state");
    check(!pane.activate() && !pane.focused && pane.selected == std::array<unsigned, 3>{0, 1, 2},
          "activation without focus does nothing");
    check(pane.focus_next() && pane.focused == 0U, "Tab entry");
    check(pane.focus_next() && pane.focused == 1U, "Tab middle");
    check(pane.focus_next() && pane.focused == 2U, "Tab last");
    check(pane.focus_next() && pane.focused == 0U, "Tab wraps");
    check(pane.selected == std::array<unsigned, 3>{0, 1, 2}, "focus leaves selections unchanged");
    check(pane.focus_next(true) && pane.focused == 2U, "Shift Tab wraps backwards");
    check(pane.focus_next(true) && pane.focused == 1U, "Shift Tab middle");
    check(pane.focus_next(true) && pane.focused == 0U, "Shift Tab first");
    pane.focused.reset();
    check(pane.focus_next(true) && pane.focused == 2U, "Shift Tab entry");
    check(pane.move_focus(1) && pane.focused == 0U, "right arrow wraps");
    check(pane.move_focus(-1) && pane.focused == 2U, "left arrow wraps");
    check(pane.move_focus(-1) && pane.focused == 1U, "left arrow middle");
    check(pane.activate() && pane.selected == std::array<unsigned, 3>{0, 2, 2},
          "activation changes only focused pane");
    for (int i = 0; i < 5; ++i) check(pane.activate(), "repeat activation succeeds");
    check(pane.selected == std::array<unsigned, 3>{0, 2, 2}, "five activations complete cycle");
    check(pane.choose(2) && pane.focused == 2U && pane.selected == std::array<unsigned, 3>{0, 2, 3},
          "mouse chooses focus and cycles one pane");
    check(pane.choose(0) && pane.focused == 0U && pane.selected == std::array<unsigned, 3>{1, 2, 3},
          "mouse selects different pane");
    const auto selected = pane.selected;
    const auto focused = pane.focused;
    for (unsigned bad : {3U, std::numeric_limits<unsigned>::max()})
        check(!pane.choose(bad) && pane.selected == selected && pane.focused == focused,
              "invalid mouse index leaves state untouched");
    for (int bad : {0, 2, -2, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()})
        check(!pane.move_focus(bad) && pane.selected == selected && pane.focused == focused,
              "invalid arrow direction leaves state untouched");
    pane.focused.reset();
    check(pane.move_focus(-1) && pane.focused == 2U, "left arrow entry");
    pane.focused.reset();
    check(pane.move_focus(1) && pane.focused == 0U, "right arrow entry");
    pane.focused = std::numeric_limits<unsigned>::max();
    check(!pane.activate() && pane.selected == selected, "invalid public focus guarded");
    check(pane.focus_next() && pane.focused == 0U, "invalid public focus recovers on navigation");
    return failures;
}
} // namespace hamclock
