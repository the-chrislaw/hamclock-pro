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
        enter("W1AW")
        let grid = app.textFields["gridField"]
        moveFocus(to: grid)
        enter("FN31pr")
        let dxGrid = app.textFields["dxGridField"]
        moveFocus(to: dxGrid)
        enter("PM95uq")
        let save = app.buttons["saveStation"]
        moveFocus(to: save)
        remote.press(.select)

        // Dashboard with the station configured.
        XCTAssertTrue(app.descendants(matching: .any)["pane0"].waitForExistence(timeout: 10), "dashboard is shown")
        let station = app.descendants(matching: .any)["stationCard"]
        XCTAssertTrue(station.label.contains("W1AW"), "station card shows the saved call: \(station.label)")
        XCTAssertTrue(station.label.contains("FN31pr"), "station card shows the DE grid")
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

    /// Owner request: a rotating pane steps with left/right on the remote (backwards and forwards),
    /// its interval restarts after a manual step, and up/down still move the focus away.
    func testRotatingPaneStepsWithLeftRight() {
        launchWithStation()
        let pane0 = app.descendants(matching: .any)["pane0"]
        let pane1 = app.descendants(matching: .any)["pane1"]
        let map = app.descendants(matching: .any)["mapCard"]

        // A pane that does not rotate: right moves the focus to the next pane, as before.
        moveFocus(to: pane0)
        remote.press(.right)
        XCTAssertTrue(waitFor { pane1.holdsFocus }, "right moves focus off a pane that does not rotate; focused: \(self.focused.exists ? self.focused.identifier : "nothing")")

        // Rotate pane 2 every 10 s.
        remote.press(.select, forDuration: 1.2)
        XCTAssertTrue(app.buttons["optRotate"].waitForExistence(timeout: 5))
        moveFocus(to: app.buttons["optRotate"])
        remote.press(.select)
        remote.press(.select)
        XCTAssertTrue(waitFor { self.app.buttons["optRotate"].label.contains("10 s") })
        remote.press(.menu)
        XCTAssertTrue(waitFor { pane1.holdsFocus }, "focus back on the pane")
        XCTAssertEqual(pane1.value as? String, "solarwind")

        remote.press(.right)
        XCTAssertTrue(waitFor { (pane1.value as? String) == "bands" }, "right steps forwards: \(String(describing: pane1.value))")
        XCTAssertTrue(pane1.holdsFocus, "the focus stays on the rotating pane")
        shot("tvos-rotating-pane-hint")
        remote.press(.left)
        remote.press(.left)
        XCTAssertTrue(waitFor { (pane1.value as? String) == "spaceweather" }, "left steps backwards")
        XCTAssertTrue(pane1.holdsFocus)
        remote.press(.left)
        XCTAssertTrue(waitFor { (pane1.value as? String) == PaneOrder.last }, "left wraps round to the last TV widget")
        XCTAssertTrue(waitFor { pane1.label.contains(PaneOrder.lastTitle) }, "the stepped widget shows at once: \(pane1.label)")
        sleep(6) // under the 10 s interval: the manual step must not be undone straight away
        XCTAssertTrue(pane1.label.contains(PaneOrder.lastTitle), "interval restarted after the step: \(pane1.label)")

        // Up/down still leave a rotating pane.
        remote.press(.down)
        XCTAssertTrue(waitFor { map.holdsFocus }, "down moves on to the map")
        remote.press(.up)
        XCTAssertTrue(waitFor { pane1.holdsFocus || pane0.holdsFocus || self.app.descendants(matching: .any)["pane2"].holdsFocus },
                      "up returns to the panes")
        moveFocus(to: app.buttons["settingsButton"])
        moveFocus(to: app.descendants(matching: .any)["stationCard"])
    }

    /// Menu on the dashboard root (nothing open) must leave the app for the Home screen: from any
    /// focus, after closing an overlay or Settings, and while dimmed. Apple rejects apps where it
    /// does not.
    func testMenuAtRootExitsToHomeScreen() {
        launchWithStation()
        menuExits("station card focused")

        launchWithStation() // fresh launch per case: no focus or overlay state carried over
        moveFocus(to: app.descendants(matching: .any)["pane1"])
        menuExits("pane focused")

        launchWithStation() // fresh launch per case: no focus or overlay state carried over
        moveFocus(to: app.descendants(matching: .any)["mapCard"])
        menuExits("map focused")

        launchWithStation() // fresh launch per case: no focus or overlay state carried over
        moveFocus(to: app.descendants(matching: .any)["mapCard"])
        remote.press(.select)
        XCTAssertTrue(app.buttons["layer_aurora"].waitForExistence(timeout: 5))
        remote.press(.menu)
        XCTAssertTrue(waitFor { !self.app.buttons["layer_aurora"].exists }, "first Menu closes the map menu")
        XCTAssertEqual(app.state, .runningForeground, "closing the overlay does not leave the app")
        menuExits("after closing the map menu")

        launchWithStation() // fresh launch per case: no focus or overlay state carried over
        moveFocus(to: app.descendants(matching: .any)["pane0"])
        remote.press(.select)
        XCTAssertTrue(app.buttons["tab0"].waitForExistence(timeout: 5))
        remote.press(.menu)
        XCTAssertTrue(waitFor { !self.app.buttons["tab0"].exists }, "first Menu closes the pane chooser")
        menuExits("after closing the pane chooser")

        launchWithStation() // fresh launch per case: no focus or overlay state carried over
        moveFocus(to: app.buttons["settingsButton"])
        remote.press(.select)
        XCTAssertTrue(app.buttons["doneButton"].waitForExistence(timeout: 5))
        remote.press(.menu)
        XCTAssertTrue(waitFor { !self.app.buttons["doneButton"].exists }, "first Menu closes Settings (app state \(self.app.state.rawValue), focus \(self.focused.debugDescription))")
        XCTAssertEqual(app.state, .runningForeground)
        menuExits("after closing Settings")

        // Dimmed (dimming on, remote idle for a day).
        launchWithStation(["-hcpDimHours", "2", "-hcpIdle"])
        shot("tvos-dimmed")
        menuExits("while dimmed")
    }

    /// Every TV widget, six at a time in the no-map layout (two rows of panes), with the test
    /// station W1AW at FN31pr and DX PM95uq (Tokyo). The focus must reach every pane and get back to Settings with the arrows alone.
    func testEveryWidgetInSixPaneLayout() {
        let pages = [["spaceweather", "solarwind", "bands", "solarscales", "solaraurora", "solardrap"],
                     ["solarimage", "ota", "weather", "quakes", "storms", "fires"],
                     ["sunmoon", "satellites", "planets", "eclipse", "launches", "none"]]
        for (n, page) in pages.enumerated() {
            app.launchArguments = ["-hcpResetSettings", "-hcpStation", "W1AW", "FN31pr", "PM95uq", "America/New_York",
                                   "-hcpMapSize", "hidden", "-hcpPanes", page.joined(separator: ",")]
            app.launch()
            let last = app.descendants(matching: .any)["pane5"]
            XCTAssertTrue(last.waitForExistence(timeout: 10), "six panes without the map")
            XCTAssertFalse(app.descendants(matching: .any)["mapCard"].exists, "no map in the six-pane layout")
            for (i, id) in page.enumerated() {
                XCTAssertEqual(app.descendants(matching: .any)["pane\(i)"].value as? String, id)
            }
            sleep(12) // feeds arrive
            shot("tvos-widgets-\(n + 1)")
            // Focus visits every pane and comes back up to Settings: no traps.
            for i in [0, 1, 2, 5, 4, 3] { moveFocus(to: app.descendants(matching: .any)["pane\(i)"]) }
            moveFocus(to: app.buttons["settingsButton"])
            app.terminate()
        }
    }

    /// Pane options (rotate, previous, empty, reset), the map menu (centre, zoom, long path) and
    /// the Settings sections: each change shows on the dashboard and survives a relaunch.
    func testOptionsMapAndSettingsPersist() {
        app.launchArguments = ["-hcpResetSettings", "-hcpStation", "W1AW", "FN31pr", "PM95uq", "America/New_York"]
        app.launch()
        let pane0 = app.descendants(matching: .any)["pane0"]
        let station = app.descendants(matching: .any)["stationCard"]
        let map = app.descendants(matching: .any)["mapCard"]
        XCTAssertTrue(pane0.waitForExistence(timeout: 10))

        // Pane options: rotation steps off -> 5 s, Previous wraps to an empty pane, Reset restores.
        openOptions(pane0)
        moveFocus(to: app.buttons["optRotate"])
        remote.press(.select)
        XCTAssertTrue(waitFor { self.app.buttons["optRotate"].label.contains("5 s") }, "rotation steps to 5 s")
        moveFocus(to: app.buttons["optPrevious"])
        remote.press(.select)
        XCTAssertTrue(waitFor { (pane0.value as? String) == "none" }, "Previous before the first widget is the empty pane")
        openOptions(pane0)
        moveFocus(to: app.buttons["optReset"])
        remote.press(.select)
        XCTAssertTrue(waitFor { (pane0.value as? String) == "spaceweather" }, "Reset all panes restores the defaults")
        openOptions(pane0)
        XCTAssertTrue(app.buttons["optRotate"].label.contains("off"), "reset also stops rotation")
        moveFocus(to: app.buttons["optEmpty"])
        remote.press(.select)
        XCTAssertTrue(waitFor { (pane0.value as? String) == "none" }, "Empty pane")

        // Map menu: DE centre, 2x, long path; the card and the station path follow.
        moveFocus(to: map)
        remote.press(.select)
        for id in ["centre_de", "zoom_2", "path_long"] {
            moveFocus(to: app.buttons[id])
            remote.press(.select)
        }
        XCTAssertTrue(app.buttons["centre_de"].isSelected && app.buttons["zoom_2"].isSelected)
        moveFocus(to: app.buttons["layer_aurora"])
        remote.press(.select)
        XCTAssertTrue(waitFor { self.app.buttons["layer_aurora"].label.contains("✓") }, "aurora overlay switched on")
        shot("tvos-map-menu-changed")
        remote.press(.menu)
        XCTAssertTrue(waitFor { (map.value as? String) == "de 2x long" }, "map shows DE, 2x, long path: \(String(describing: map.value))")
        XCTAssertTrue(station.label.contains("Long path"), "station card shows the long path")

        // Settings: light, amber accent, 12 h clock, dimming, no map.
        moveFocus(to: app.buttons["settingsButton"])
        remote.press(.select)
        for id in ["appearance_1", "accent_2", "clock_1", "dim_1", "mapSize_1"] {
            moveFocus(to: app.buttons[id])
            remote.press(.select)
            XCTAssertTrue(waitFor { self.app.buttons[id].isSelected }, "\(id) selected")
        }
        shot("tvos-settings-light")
        remote.press(.menu)
        XCTAssertTrue(app.descendants(matching: .any)["pane5"].waitForExistence(timeout: 5), "six panes once the map is off")
        XCTAssertFalse(map.exists, "map hidden")
        XCTAssertTrue(station.label.contains("AM") || station.label.contains("PM"), "12 h clock: \(station.label)")
        sleep(8)
        shot("tvos-light-six-panes")

        // Relaunch: everything above was kept.
        app.terminate()
        app.launchArguments = []
        app.launch()
        XCTAssertTrue(app.descendants(matching: .any)["pane5"].waitForExistence(timeout: 10), "no-map layout kept")
        XCTAssertEqual(pane0.value as? String, "none", "empty pane kept")
        XCTAssertTrue(station.label.contains("AM") || station.label.contains("PM"), "12 h clock kept")
        moveFocus(to: app.buttons["settingsButton"])
        remote.press(.select)
        for id in ["appearance_1", "accent_2", "clock_1", "dim_1", "mapSize_1"] {
            XCTAssertTrue(app.buttons[id].waitForExistence(timeout: 5) && app.buttons[id].isSelected, "\(id) kept")
        }
        moveFocus(to: app.buttons["mapSize_0"])
        remote.press(.select)
        remote.press(.menu)
        XCTAssertTrue(map.waitForExistence(timeout: 5), "map back")
        XCTAssertEqual(map.value as? String, "de 2x long", "map centre, zoom and path kept across relaunch")
        XCTAssertTrue(station.label.contains("Long path"), "long path kept")
    }

    /// Dst index (ported from the Qt widget): offered in the chooser, shows a reading and a badge.
    func testDstIndexPane() {
        app.launchArguments = ["-hcpResetSettings", "-hcpStation", "W1AW", "FN31pr", "PM95uq", "America/New_York",
                               "-hcpPanes", "solardst"]
        app.launch()
        let pane0 = app.descendants(matching: .any)["pane0"]
        XCTAssertTrue(pane0.waitForExistence(timeout: 20))
        XCTAssertEqual(pane0.value as? String, "solardst")
        XCTAssertTrue(waitFor(20) { pane0.label.contains("nT") || pane0.label.contains("Offline") }, "Dst reading or an honest Offline: \(pane0.label)")
        shot("tvos-dst")
        moveFocus(to: pane0)
        remote.press(.select)
        XCTAssertTrue(app.buttons["choose_solardst"].waitForExistence(timeout: 5), "Dst index is offered on TV")
    }

    /// Space alerts (ported from the Qt widget): offered in the chooser; shows what is in force, or
    /// says nothing is, or an honest Offline / No data.
    func testSpaceAlertsPane() {
        app.launchArguments = ["-hcpResetSettings", "-hcpStation", "W1AW", "FN31pr", "PM95uq", "America/New_York",
                               "-hcpPanes", "solaralerts"]
        app.launch()
        let pane0 = app.descendants(matching: .any)["pane0"]
        XCTAssertTrue(pane0.waitForExistence(timeout: 20))
        XCTAssertEqual(pane0.value as? String, "solaralerts")
        XCTAssertTrue(waitFor(20) {
            ["until", "issued", "in force", "Offline", "No data"].contains { pane0.label.contains($0) }
        }, "alerts, none in force, or an honest Offline: \(pane0.label)")
        shot("tvos-space-alerts")
        moveFocus(to: pane0)
        remote.press(.select)
        XCTAssertTrue(app.buttons["choose_solaralerts"].waitForExistence(timeout: 5), "Space alerts is offered on TV")
    }

    /// Solar trend (ported from the Qt widget): offered in the chooser; shows the flux trend and
    /// sunspots, or an honest Offline / No data.
    func testSolarTrendPane() {
        app.launchArguments = ["-hcpResetSettings", "-hcpStation", "W1AW", "FN31pr", "PM95uq", "America/New_York",
                               "-hcpPanes", "solartrend"]
        app.launch()
        let pane0 = app.descendants(matching: .any)["pane0"]
        XCTAssertTrue(pane0.waitForExistence(timeout: 20))
        XCTAssertEqual(pane0.value as? String, "solartrend")
        XCTAssertTrue(waitFor(20) {
            ["Sunspots", "over", "Offline", "No data"].contains { pane0.label.contains($0) }
        }, "trend data or an honest Offline: \(pane0.label)")
        shot("tvos-solar-trend")
        moveFocus(to: pane0)
        remote.press(.select)
        XCTAssertTrue(app.buttons["choose_solartrend"].waitForExistence(timeout: 5), "Solar trend is offered on TV")
    }

    /// HamQSL conditions (ported from the Qt widget): offered in the chooser; shows N0NBH's band
    /// table, or an honest Offline / No data.
    func testHamqslPane() {
        app.launchArguments = ["-hcpResetSettings", "-hcpStation", "W1AW", "FN31pr", "PM95uq", "America/New_York",
                               "-hcpPanes", "solarhamqsl"]
        app.launch()
        let pane0 = app.descendants(matching: .any)["pane0"]
        XCTAssertTrue(pane0.waitForExistence(timeout: 20))
        XCTAssertEqual(pane0.value as? String, "solarhamqsl")
        XCTAssertTrue(waitFor(20) {
            ["VHF", "No report", "Offline", "No data"].contains { pane0.label.contains($0) }
        }, "band table or an honest Offline: \(pane0.label)")
        shot("tvos-hamqsl")
        moveFocus(to: pane0)
        remote.press(.select)
        XCTAssertTrue(app.buttons["choose_solarhamqsl"].waitForExistence(timeout: 5), "HamQSL is offered on TV")
    }

    /// Satellite pass alerts: switched on, lead time and one bird changed in Settings with the
    /// remote; the choices survive a relaunch. (Whether a banner shows depends on the sky.)
    func testSatellitePassAlertSettings() {
        launchWithStation()
        moveFocus(to: app.buttons["settingsButton"])
        remote.press(.select)
        let on = app.buttons["satEnabled_1"]
        XCTAssertTrue(on.waitForExistence(timeout: 5), "pass alert settings shown")
        moveFocus(to: on)
        remote.press(.select)
        XCTAssertTrue(waitFor { on.isSelected }, "alerts on")
        let lead = app.buttons["satLead_4"]
        XCTAssertTrue(lead.waitForExistence(timeout: 5), "lead time choices appear once on")
        moveFocus(to: lead)
        remote.press(.select)
        XCTAssertTrue(waitFor { lead.isSelected }, "30 min before")
        let iss = app.buttons["sat_25544"]
        XCTAssertTrue(iss.waitForExistence(timeout: 5) && iss.isSelected, "ISS selected under All workable")
        moveFocus(to: iss)
        remote.press(.select)
        XCTAssertTrue(waitFor { !iss.isSelected }, "ISS switched off")
        shot("tvos-settings-pass-alerts")
        remote.press(.menu)

        app.terminate()
        app.launchArguments = []
        app.launch()
        XCTAssertTrue(app.descendants(matching: .any)["pane0"].waitForExistence(timeout: 20))
        moveFocus(to: app.buttons["settingsButton"])
        remote.press(.select)
        XCTAssertTrue(app.buttons["satEnabled_1"].waitForExistence(timeout: 5))
        XCTAssertTrue(app.buttons["satEnabled_1"].isSelected && app.buttons["satLead_4"].isSelected, "kept across relaunch")
        let issAgain = app.buttons["sat_25544"]
        XCTAssertTrue(issAgain.waitForExistence(timeout: 5), "bird grid present")
        XCTAssertFalse(issAgain.isSelected, "ISS stays off")
    }

    /// Passes (ported from the Qt widget): the next 48 h of passes at DE from the amateur elements,
    /// or an honest Loading / Offline; pane options cycle the filter (All, Visible, Alerts), kept
    /// across a relaunch.
    func testPassesPane() {
        launchWithStation(["-hcpPanes", "passes"])
        let pane0 = app.descendants(matching: .any)["pane0"]
        XCTAssertEqual(pane0.value as? String, "passes")
        XCTAssertTrue(waitFor(30) {
            ["°", "No passes", "Waiting for satellite elements", "Computing", "Offline", "Loading"].contains { pane0.label.contains($0) }
        }, "pass rows or an honest state: \(pane0.label)")
        shot("tvos-passes")
        openOptions(pane0)
        let filter = app.buttons["optPassesFilter"]
        XCTAssertTrue(filter.waitForExistence(timeout: 5), "Passes pane offers its filter")
        XCTAssertTrue(filter.label.contains("All"), filter.label)
        moveFocus(to: filter)
        remote.press(.select)
        XCTAssertTrue(waitFor { filter.label.contains("Visible") }, filter.label)
        remote.press(.select)
        XCTAssertTrue(waitFor { filter.label.contains("Alerts") }, filter.label)
        remote.press(.menu)
        XCTAssertTrue(waitFor(10) { pane0.label.contains("Alerts") }, "status names the filter: \(pane0.label)")

        app.terminate()
        app.launchArguments = []
        app.launch()
        XCTAssertTrue(pane0.waitForExistence(timeout: 20))
        XCTAssertTrue(waitFor(10) { pane0.label.contains("Alerts") }, "filter kept across relaunch: \(pane0.label)")
        moveFocus(to: pane0)
        remote.press(.select)
        XCTAssertTrue(app.buttons["choose_passes"].waitForExistence(timeout: 5), "Passes is offered on TV")
    }

    /// HF path reliability (ported from the Qt bandsmatrix widget): the DE → DX band × 24 h grid
    /// with the best band now, or an honest Loading / Offline / No data.
    func testPathReliabilityPane() {
        launchWithStation(["-hcpPanes", "bandsmatrix"])
        let pane0 = app.descendants(matching: .any)["pane0"]
        XCTAssertEqual(pane0.value as? String, "bandsmatrix")
        XCTAssertTrue(waitFor(30) {
            ["Best now", "HF closed now", "Waiting for SFI", "Offline", "No data"].contains { pane0.label.contains($0) }
        }, "reliability grid or an honest state: \(pane0.label)")
        XCTAssertFalse(pane0.label.contains("Not available on Apple TV"), pane0.label)
        shot("tvos-path-reliability")
        moveFocus(to: pane0)
        remote.press(.select)
        XCTAssertTrue(app.buttons["choose_bandsmatrix"].waitForExistence(timeout: 5), "HF path reliability is offered on TV")
    }

    /// Space weather impact (ported from the Qt solarimpact widget): R, S, G (and fresh Dst) ranked
    /// worst first with their effects, or an honest Loading / Offline / No data.
    func testImpactPane() {
        launchWithStation(["-hcpPanes", "solarimpact"])
        let pane0 = app.descendants(matching: .any)["pane0"]
        XCTAssertEqual(pane0.value as? String, "solarimpact")
        XCTAssertTrue(waitFor(30) {
            ["All quiet", "HF", "Aurora", "radiation", "Offline", "No data"].contains { pane0.label.contains($0) }
        }, "ranked effects or an honest state: \(pane0.label)")
        XCTAssertFalse(pane0.label.contains("Not available on Apple TV"), pane0.label)
        shot("tvos-impact")
        moveFocus(to: pane0)
        remote.press(.select)
        XCTAssertTrue(app.buttons["choose_solarimpact"].waitForExistence(timeout: 5), "Space weather impact is offered on TV")
    }

    // MARK: helpers

    /// Fresh launch with the test station (W1AW, FN31pr, DX PM95uq); waits until the dashboard
    /// exists AND the focus engine has put the focus somewhere (slower on older Simulators).
    private func launchWithStation(_ extra: [String] = []) {
        app.terminate()
        app.launchArguments = ["-hcpResetSettings", "-hcpStation", "W1AW", "FN31pr", "PM95uq", "America/New_York"] + extra
        app.launch()
        XCTAssertTrue(app.descendants(matching: .any)["pane0"].waitForExistence(timeout: 20), "dashboard")
        XCTAssertTrue(waitFor(10) { self.focused.exists }, "the focus engine has a focused element")
    }

    /// Menu with nothing open sends the app to the background (tvOS Home screen).
    private func menuExits(_ when: String, file: StaticString = #filePath, line: UInt = #line) {
        XCTAssertEqual(app.state, .runningForeground, "in the app before Menu (\(when))", file: file, line: line)
        remote.press(.menu)
        XCTAssertTrue(app.wait(for: .runningBackground, timeout: 5), "Menu at the root exits (\(when))", file: file, line: line)
    }

    /// Hold Select on a pane: its options open.
    private func openOptions(_ pane: XCUIElement) {
        moveFocus(to: pane)
        remote.press(.select, forDuration: 1.2)
        XCTAssertTrue(app.buttons["optRotate"].waitForExistence(timeout: 5), "pane options open")
    }

    /// Types into the focused text field through the tvOS keyboard, then returns.
    private func enter(_ text: String) {
        remote.press(.select)
        sleep(1)
        app.typeText(text)
        remote.press(.menu)
        sleep(1)
    }

    private var focused: XCUIElement {
        app.descendants(matching: .any).matching(XCUIElement.focusedPredicate).firstMatch
    }

    /// Moves the focus engine toward `target` with the remote's arrows, by comparing frames.
    private func moveFocus(to target: XCUIElement, file: StaticString = #filePath, line: UInt = #line) {
        XCTAssertTrue(target.waitForExistence(timeout: 5), "target exists", file: file, line: line)
        _ = waitFor(5) { self.focused.exists } // just after a launch or an overlay closing
        var stuck = 0
        for _ in 0..<40 {
            if target.holdsFocus { return }
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
        if !target.holdsFocus {
            shot("failure-focus")
            print("FOCUSED: \(focused.debugDescription)\nTREE: \(app.debugDescription)")
        }
        XCTAssertTrue(target.holdsFocus, "focus reached \(target); focused: \(focused.exists ? focused.identifier + " " + focused.label : "nothing")", file: file, line: line)
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

/// The last TV widget in the chooser's registry order (WidgetCatalog.onTV), where a backwards
/// step from the first one wraps to.
private enum PaneOrder {
    static let last = "launches"
    static let lastTitle = "Launches"
}

extension XCUIElement {
    static let focusedPredicate = NSPredicate(format: "hasFocus == true")

    /// The element has the focus, or one of its descendants reports it. tvOS 18 reports
    /// `hasFocus` on a text inside a focused SwiftUI Button (e.g. "Weakly south" in the Solar
    /// wind pane) rather than on the Button itself; tvOS 26+ reports it on the Button.
    var holdsFocus: Bool {
        hasFocus || descendants(matching: .any).matching(Self.focusedPredicate).firstMatch.exists
    }
}
