# Native pane keyboard controller

Original C++17, SDL-independent controller for three instrument panes, each with five choices. The default selections are `0`, `1`, and `2`; no pane initially has keyboard focus.

- `focus_next()` supports Tab, entering pane 0 and cycling forward with wrap. `focus_next(true)` supports Shift-Tab, entering pane 2 and cycling backward with wrap.
- `move_focus(1)` supports right-arrow navigation; `move_focus(-1)` supports left-arrow navigation. With no focus, these enter the first/last pane respectively. Other direction values return false without changes.
- `activate()` advances only the focused pane's choice, wrapping after choice 4. Without valid focus it returns false without changes.
- `choose(index)` retains click behavior: a valid index 0–2 sets focus and advances that pane's choice. Invalid indices return false without changes.

Selection changes and focus changes are separate operations. This controller does not interpret key events, render an outline, provide an accessibility tree, or claim WCAG conformance. Application integration must map keys, draw a visible focus indicator, and avoid intercepting keyboard input in other controls. Native screen-reader support remains unresolved.

Because the structure exposes its state, activation guards invalid public focus values; navigation treats those as no focus. Choice cycling normalizes its value modulo five without unsigned overflow. Normal application code should keep focus and selection state within the documented bounds.

`run_pane_control_tests()` returns a failure count. Standalone tests cover Tab/Shift-Tab entry and wrapping, arrow navigation and invalid directions, activation affecting one pane, five-choice cycles, click focus/activation, invalid indices with unchanged state, and defensive handling of invalid public focus. These tests require no window or display.
