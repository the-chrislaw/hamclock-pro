import XCTest

/// Drives the app with the Siri Remote only (XCUIRemote) on the tvOS Simulator:
/// first-run setup -> dashboard -> pane chooser -> pane options -> a settings change that must
/// survive a relaunch. With HCP_SHOT_DIR set (TEST_RUNNER_HCP_SHOT_DIR=... xcodebuild test) it
/// also writes 1920x1080 screenshots of each screen there.
final class RemoteWalkthroughTests: XCTestCase {
    private let remote = XCUIRemote.shared
    private var app: XCUIApplication!

    override func setUp() {
        continueAfterFailure = false
        app = XCUIApplication()
    }

    func testRemoteWalkthrough() {
        app.launchArguments = ["-hcpResetSettings"]
        app.launch()

        // First run: the setup screen opens with the callsign field focused.
        let callsign = app.textFields["callsignField"]
        XCTAssertTrue(callsign.waitForExistence(timeout: 10), "first-run setup is shown")
        shot("tvos-first-run")
        moveFocus(to: callsign)
        enter("N0CALL")
        let grid = app.textFields["gridField"]
        moveFocus(to: grid)
        enter("CN85pm")
        let dxGrid = app.textFields["dxGridField"]
        moveFocus(to: dxGrid)
        enter("JN58td")
        let save = app.buttons["saveStation"]
        moveFocus(to: save)
        remote.press(.select)

        // Dashboard with the station configured.
        XCTAssertTrue(app.descendants(matching: .any)["pane0"].waitForExistence(timeout: 10), "dashboard is shown")
        let station = app.descendants(matching: .any)["stationCard"]
        XCTAssertTrue(station.label.contains("N0CALL"), "station card shows the saved call: \(station.label)")
        XCTAssertTrue(station.label.contains("CN85pm"), "station card shows the DE grid")
        sleep(6) // let the feeds arrive for the screenshot
        shot("tvos-dashboard")

        // Select on a pane opens the chooser; pick Planets.
        let pane0 = app.descendants(matching: .any)["pane0"]
        moveFocus(to: pane0)
        remote.press(.select)
        // It opens on the current widget's tab (Space Wx); Sky & Space lists Planets.
        XCTAssertTrue(app.buttons["choose_spaceweather"].waitForExistence(timeout: 5), "pane chooser opens")
        XCTAssertTrue(app.buttons["tab1"].isSelected, "chooser opens on the current widget's category")
        shot("tvos-chooser")
        moveFocus(to: app.buttons["tab4"])
        remote.press(.select)
        let planets = app.buttons["choose_planets"]
        XCTAssertTrue(planets.waitForExistence(timeout: 5), "Sky & Space lists Planets")
        moveFocus(to: app.buttons["tab0"])
        remote.press(.select)
        XCTAssertTrue(app.buttons["choose_fires"].waitForExistence(timeout: 5), "All lists every TV widget")
        XCTAssertFalse(app.buttons["choose_dxcluster"].exists, "chooser hides widgets without a TV source")
        moveFocus(to: planets)
        remote.press(.select)
        XCTAssertTrue(waitFor { (pane0.value as? String) == "planets" }, "pane 1 now shows Planets")

        // Long-press Select opens the pane options; turn rotation on, then step to the next widget.
        moveFocus(to: pane0)
        remote.press(.select, forDuration: 1.2)
        let rotate = app.buttons["optRotate"]
        XCTAssertTrue(rotate.waitForExistence(timeout: 5), "pane options open on long press")
        shot("tvos-pane-options")
        moveFocus(to: app.buttons["optNext"])
        remote.press(.select)
        XCTAssertTrue(waitFor { (pane0.value as? String) == "eclipse" }, "Next widget steps in registry order")

        // Play/Pause also opens the options; Menu closes them.
        moveFocus(to: pane0)
        remote.press(.playPause)
        XCTAssertTrue(app.buttons["optReset"].waitForExistence(timeout: 5), "Play/Pause opens pane options")
        remote.press(.menu)
        XCTAssertTrue(waitFor { !self.app.buttons["optReset"].exists }, "Menu closes the overlay")

        // Map menu from the map card.
        let map = app.descendants(matching: .any)["mapCard"]
        moveFocus(to: map)
        remote.press(.select)
        XCTAssertTrue(app.buttons["layer_aurora"].waitForExistence(timeout: 5), "map menu opens")
        shot("tvos-map-menu")
        remote.press(.menu)

        // Settings: switch to imperial units.
        moveFocus(to: app.buttons["settingsButton"])
        remote.press(.select)
        let imperial = app.buttons["units_1"]
        XCTAssertTrue(imperial.waitForExistence(timeout: 5), "settings open")
        shot("tvos-settings")
        moveFocus(to: imperial)
        remote.press(.select)
        XCTAssertTrue(waitFor { imperial.isSelected }, "imperial units selected")
        shot("tvos-settings-display")
        remote.press(.menu)
        XCTAssertTrue(pane0.waitForExistence(timeout: 5))

        // Relaunch: no setup screen, the pane choice and the units are kept.
        app.terminate()
        app.launchArguments = []
        app.launch()
        XCTAssertTrue(app.descendants(matching: .any)["pane0"].waitForExistence(timeout: 10), "dashboard, not setup, after relaunch")
        XCTAssertFalse(app.textFields["callsignField"].exists)
        XCTAssertEqual(app.descendants(matching: .any)["pane0"].value as? String, "eclipse")
        moveFocus(to: app.buttons["settingsButton"])
        remote.press(.select)
        XCTAssertTrue(app.buttons["units_1"].waitForExistence(timeout: 5))
        XCTAssertTrue(app.buttons["units_1"].isSelected, "imperial units persisted across relaunch")
    }

    // MARK: helpers

    /// Types into the focused text field through the tvOS keyboard, then returns.
    private func enter(_ text: String) {
        remote.press(.select)
        sleep(1)
        app.typeText(text)
        remote.press(.menu)
        sleep(1)
    }

    private var focused: XCUIElement {
        app.descendants(matching: .any).element(matching: NSPredicate(format: "hasFocus == true"))
    }

    /// Moves the focus engine toward `target` with the remote's arrows, by comparing frames.
    private func moveFocus(to target: XCUIElement, file: StaticString = #filePath, line: UInt = #line) {
        XCTAssertTrue(target.waitForExistence(timeout: 5), "target exists", file: file, line: line)
        var stuck = 0
        for _ in 0..<40 {
            if target.hasFocus { return }
            let from = focused.exists ? focused.frame : .zero
            let to = target.frame
            let dx = to.midX - from.midX, dy = to.midY - from.midY
            let vertical: XCUIRemote.Button? = to.minY >= from.maxY - 1 ? .down : to.maxY <= from.minY + 1 ? .up : nil
            let horizontal: XCUIRemote.Button? = dx > 1 ? .right : dx < -1 ? .left : nil
            // Prefer the dominant axis; when a press did not move the focus, try the other one.
            let primary = abs(dy) >= abs(dx) * 0.3 ? (vertical ?? horizontal) : (horizontal ?? vertical)
            let secondary = primary == vertical ? horizontal : vertical
            remote.press((stuck % 2 == 1 ? secondary : primary) ?? (dy > 0 ? .down : .up))
            usleep(350_000)
            stuck = !focused.exists || focused.frame == from ? stuck + 1 : 0
        }
        if !target.hasFocus {
            shot("failure-focus")
            print("FOCUSED: \(focused.debugDescription)\nTREE: \(app.debugDescription)")
        }
        XCTAssertTrue(target.hasFocus, "focus reached \(target)", file: file, line: line)
    }

    private func waitFor(_ timeout: TimeInterval = 5, _ condition: @escaping () -> Bool) -> Bool {
        let end = Date().addingTimeInterval(timeout)
        while Date() < end {
            if condition() { return true }
            usleep(200_000)
        }
        return condition()
    }

    private func shot(_ name: String) {
        let image = XCUIScreen.main.screenshot()
        let attachment = XCTAttachment(screenshot: image)
        attachment.name = name
        attachment.lifetime = .keepAlways
        add(attachment)
        if let dir = ProcessInfo.processInfo.environment["HCP_SHOT_DIR"], !dir.isEmpty {
            try? image.pngRepresentation.write(to: URL(fileURLWithPath: dir).appendingPathComponent(name + ".png"))
        }
    }
}
