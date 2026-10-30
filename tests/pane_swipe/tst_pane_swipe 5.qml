import QtQuick
import QtTest
import HamClock

// Drives the real PaneSlot.qml (copied in by tests/pane_swipe_regression.py with
// stub settings and widgets) with mouse drags and keys. Only timing-independent
// cases: every drag holds still before release, so no flick is ever measured;
// the flick and velocity maths are covered by tests/pager_regression.cpp.
Rectangle {
    id: root
    width: 390
    height: 300
    color: Theme.canvas
    PaneSlot { id: slot; x: 10; y: 10; width: 370; height: 280 }
    SignalSpy { id: chooseSpy; target: slot; signalName: "chooseRequested" }

    TestCase {
        name: "PaneSwipe"
        when: windowShown
        readonly property int holdMs: 200

        function initTestCase() { Theme.phone = true }
        function init() {
            Stubs.settings.panes = ["w2", "w1"]
            chooseSpy.clear()
            tryCompare(slot, "moving", false)
        }
        // Drags by dx in 10 px steps, holds still, releases, and waits for the slide to land.
        function dragBy(dx, startX) {
            let x = startX || 300
            mousePress(root, x, 150)
            const step = dx < 0 ? -10 : 10
            for (let moved = 0; Math.abs(moved) < Math.abs(dx); moved += step) {
                x += step
                mouseMove(root, x, 150, 10, Qt.LeftButton)
            }
            mouseMove(root, x, 150, holdMs, Qt.LeftButton)
            mouseRelease(root, x, 150, Qt.LeftButton)
            tryCompare(slot, "moving", false)
        }

        function test_neighbours_are_loaded_before_a_swipe() {
            // w2's neighbours are w1 and w3: three live cards, so nothing slides in blank.
            verify(Stubs.registry.created >= 3, "cards built: " + Stubs.registry.created)
        }
        function test_card_follows_finger_with_neighbour_beside_it() {
            mousePress(root, 300, 150)
            for (let i = 1; i <= 12; ++i) mouseMove(root, 300 - 10 * i, 150, 10, Qt.LeftButton)
            verify(slot.offset < -80 && slot.offset > -130, "offset " + slot.offset)
            compare(slot.shownIncoming, "w3")
            mouseMove(root, 180, 150, holdMs, Qt.LeftButton)
            mouseRelease(root, 180, 150, Qt.LeftButton)
            tryCompare(slot, "moving", false)
        }
        function test_slow_drag_past_a_third_commits() {
            const calls = Stubs.settings.setCalls
            dragBy(-170)
            compare(slot.choice, "w3")
            compare(slot.offset, 0)
            compare(Stubs.settings.setCalls, calls + 1)
        }
        function test_short_slow_drag_springs_back() {
            const calls = Stubs.settings.setCalls
            dragBy(-90)
            compare(slot.choice, "w2")
            compare(slot.offset, 0)
            compare(Stubs.settings.setCalls, calls)
        }
        function test_right_drag_goes_to_previous() {
            dragBy(170, 80)
            compare(slot.choice, "w1")
        }
        function test_wraps_through_the_empty_page() {
            Stubs.settings.panes = ["w1", "w2"]
            dragBy(170, 80)
            compare(slot.choice, "none")
        }
        function test_keys_slide_and_land() {
            slot.forceActiveFocus()
            keyClick(Qt.Key_Right)
            verify(slot.moving, "a key step slides")
            tryCompare(slot, "moving", false)
            compare(slot.choice, "w3")
            keyClick(Qt.Key_Left)
            tryCompare(slot, "moving", false)
            compare(slot.choice, "w2")
            keyClick(Qt.Key_Right)
            keyClick(Qt.Key_Right)  // a second press lands the first slide, then starts its own
            tryCompare(slot, "moving", false)
            compare(slot.choice, "w4")
        }
        function test_vertical_drag_does_not_page() {
            mousePress(root, 200, 60)
            for (let i = 1; i <= 10; ++i) mouseMove(root, 200 + i, 60 + 15 * i, 10, Qt.LeftButton)
            mouseRelease(root, 210, 210, Qt.LeftButton)
            tryCompare(slot, "moving", false)
            compare(slot.choice, "w2")
        }
        function test_tap_still_opens_the_chooser() {
            mouseClick(root, 200, 150)
            tryCompare(chooseSpy, "count", 1)
        }
    }
}
