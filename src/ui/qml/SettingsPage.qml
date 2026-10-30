import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HamClock

// Settings: station, accounts (sign-ins and the DX cluster node), appearance and feeds.
// Changes apply as they are made: choices at once, typed fields when editing finishes.
// There are no Save buttons; an input that can't be used shows the error from C++ in its card.
Rectangle {
    id: page
    color: Theme.canvas
    signal closeRequested()

    // Done and Escape: saves what is still being typed, then closes. On an error the page
    // stays open and scrolls to that section so the message is visible.
    function saveAndClose() {
        for (const section of [stationCard, clusterCard, hardwareCard]) {
            if (!section.save()) {
                flick.contentY = Math.max(0, Math.min(flick.contentHeight - flick.height,
                                                      section.mapToItem(column, 0, 0).y - Theme.pad))
                return
            }
        }
        page.closeRequested()
    }

    // Called each time Settings opens: every field shows what is saved again, so a
    // restored configuration is not overwritten by fields typed in before it.
    function reload() {
        const bound = [
            [callsign, () => appSettings.callsign], [pskMatch, () => appSettings.pskMatch],
            [udpPort, () => appSettings.udpSpotPort], [udpGroup, () => appSettings.udpSpotGroup],
            [aprsHost, () => appSettings.aprsHost], [aprsPort, () => appSettings.aprsPort], [aprsRadius, () => appSettings.aprsRadiusKm],
            [meshHost, () => appSettings.meshHost], [meshPort, () => appSettings.meshPort], [pskMax, () => appSettings.pskMaxKm],
            [adifPath, () => appSettings.adifPath], [otaProgram, () => appSettings.otaProgram], [otaBand, () => appSettings.otaBand],
            [otaMode, () => appSettings.otaMode], [otaSort, () => appSettings.otaSort], [otaAge, () => appSettings.otaMaxAgeMin],
            [watchList, () => appSettings.watchListText], [displaySection.bannerTitle, () => appSettings.bannerTitle],
            [radioHost, () => appSettings.radioHost], [radioPort, () => appSettings.radioPort],
            [rotatorHost, () => appSettings.rotatorHost], [rotatorPort, () => appSettings.rotatorPort],
            [adsbHost, () => appSettings.adsbHost], [adsbPort, () => appSettings.adsbPort],
            [gpsHost, () => appSettings.gpsHost], [gpsPort, () => appSettings.gpsPort],
            [gpsDevice, () => appSettings.gpsDevice], [gpsBaud, () => appSettings.gpsBaud]
        ]
        for (const [field, value] of bound) {
            field.text = Qt.binding(value)
            field.edited = false
        }
        for (const [field, value] of [[deLat, () => appSettings.deLatitude], [deLon, () => appSettings.deLongitude],
                                      [dxLat, () => appSettings.dxLatitude], [dxLon, () => appSettings.dxLongitude]])
            field.text = Qt.binding(() => appSettings.configured ? value() : "")
        zone.zoneId = Qt.binding(() => timeZones.stationZone(appSettings.deZone, appSettings.configured ? appSettings.deLatitude : NaN,
                                                             appSettings.configured ? appSettings.deLongitude : NaN))
        dxZone.zoneId = Qt.binding(() => appSettings.dxZoneOverride)
        for (const cleared of [place, deGrid]) cleared.text = ""
        placeStatus.text = ""
        zoneHint.text = ""
        accounts.reload()
        for (const message of [stationError, clusterError, hardwareError]) message.text = ""
        satAlerts.refreshSystemState()
        flick.contentY = 0
    }

    function focusFirstField() { callsign.field.forceActiveFocus() }

    // Keeps the focused field above the on-screen keyboard.
    function revealFocused() {
        const item = page.Window.activeFocusItem
        let inside = false
        for (let p = item; p && !inside; p = p.parent) inside = p === column
        if (!inside) return
        const top = item.mapToItem(column, 0, 0).y
        if (top + item.height > flick.contentY + flick.height)
            flick.contentY = Math.min(flick.contentHeight - flick.height, top + item.height - flick.height + Theme.pad)
        else if (top < flick.contentY)
            flick.contentY = Math.max(0, top - Theme.pad)
    }
    Connections { target: page.Window.window; function onActiveFocusItemChanged() { Qt.callLater(page.revealFocused) } }
    Connections { target: Theme; function onKeyboardInsetChanged() { Qt.callLater(page.revealFocused) } }

    // --settings: "accounts", "display", "screen", "spots" or "hardware" scrolls to that section;
    // "accounts:hamqth" also opens that account.
    function show(target) { showTimer.target = target; showTimer.restart() }
    // After the first layout pass, so the section's position is known.
    Timer {
        id: showTimer
        property string target: ""
        interval: 50
        onTriggered: {
            const sections = { accounts: accounts, display: displaySection, screen: screenSection, spots: spotsCard, hardware: hardwareSection }
            const item = sections[target.split(":")[0]] || accounts
            flick.contentY = Math.max(0, Math.min(flick.contentHeight - flick.height, item.mapToItem(column, 0, 0).y - Theme.pad))
            const parts = target.split(":")
            if (parts[0] === "accounts" && parts.length > 1) accounts.open(parts[1])
        }
    }

    function num(text) { const v = Number(text.trim()); return text.trim().length && isFinite(v) ? v : NaN }

    // Title bar stays put while the sections scroll, so Done is always in reach.
    Rectangle {
        id: titleBar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: titleRow.implicitHeight + 2 * Theme.pad
        color: Theme.canvas
        z: 1
        RowLayout {
            id: titleRow
            width: column.width
            x: column.x
            anchors.verticalCenter: parent.verticalCenter
            Text {
                text: "Settings"
                color: Theme.ink
                font.pixelSize: Theme.fontValue
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            // Done keeps what was typed (iOS convention); it stays open only to show an error.
            AppButton { text: "Done"; primary: true; onClicked: page.saveAndClose() }
        }
        Rectangle { anchors { left: parent.left; right: parent.right; bottom: parent.bottom } height: 1; color: Theme.divider
                    visible: flick.contentY > 0 }
    }

    Flickable {
        id: flick
        anchors { top: titleBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom; bottomMargin: Theme.keyboardInset }
        contentHeight: column.implicitHeight + 2 * Theme.pad
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollIndicator.vertical: ScrollIndicator {}

        // Mouse wheel / trackpad scrolling (Flickable only handles touch drags).
        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: function (event) {
                const dy = event.pixelDelta.y !== 0 ? event.pixelDelta.y : event.angleDelta.y / 2
                flick.contentY = Math.max(0, Math.min(flick.contentHeight - flick.height, flick.contentY - dy))
            }
        }

        ColumnLayout {
            id: column
            width: Math.min(flick.width - 2 * Theme.pad, 640 * Theme.scale)
            x: Math.max(Theme.pad, (flick.width - width) / 2)
            y: 0
            spacing: Theme.gap

            Card {
                title: "Station"
                Layout.fillWidth: true
                ColumnLayout {
                    id: stationCard
                    width: parent.width
                    spacing: Theme.gap * 0.8
                    // The saved station file could not be read: say why instead of looking new.
                    Text { objectName: "stationLoadError"; visible: appSettings.loadError.length > 0
                           text: appSettings.loadError + ". Saving here replaces it; the old file is kept as station.conf.unreadable."
                           color: Theme.bad; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    FormField { id: callsign; label: "Callsign"; text: appSettings.callsign; placeholder: "e.g. W1AW"
                                field.inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: place; label: "Find a place (address or city; sent to OpenStreetMap when you press Find)"
                                    placeholder: "e.g. Portland, Oregon"
                                    Keys.onReturnPressed: findButton.clicked() }
                        AppButton { id: findButton; text: "Find"; Layout.alignment: Qt.AlignBottom
                                    onClicked: { placeStatus.text = "Searching..."; locationTools.search(place.text) } }
                    }
                    // Apple devices only: one-shot fix from the OS location service.
                    AppButton { objectName: "useMyLocation"; visible: deviceLocation.available; text: "Use my location"
                                onClicked: { placeStatus.text = "Locating..."; deviceLocation.request() } }
                    Connections {
                        target: deviceLocation
                        function onLocated(lat, lon) {
                            deLat.text = lat.toFixed(4)
                            deLon.text = lon.toFixed(4)
                            deGrid.text = locationTools.gridFor(lat, lon)
                            placeStatus.text = "Using your current location."
                            stationCard.suggestZone(lat, lon)
                        }
                        function onFailed(reason) { placeStatus.text = reason }
                    }
                    Text { id: placeStatus; visible: text.length > 0; color: Theme.secondary
                           font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Connections {
                        target: locationTools
                        function onSearchFinished(results, error) {
                            if (error.length) { placeStatus.text = error; return }
                            const r = results[0]
                            deLat.text = r.latitude.toFixed(4)
                            deLon.text = r.longitude.toFixed(4)
                            deGrid.text = locationTools.gridFor(r.latitude, r.longitude)
                            placeStatus.text = "Using: " + r.name
                            stationCard.suggestZone(r.latitude, r.longitude)
                            stationCard.autoSave()
                        }
                    }
                    FormField { id: deGrid; label: "Or grid square (fills the home coordinates)"
                                placeholder: "e.g. FN31pr"
                                field.inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                                Connections {
                                    target: deGrid.field
                                    function onEditingFinished() {
                                        const g = locationTools.fromGrid(deGrid.text)
                                        if (g.latitude === undefined) return
                                        deLat.text = g.latitude.toFixed(4)
                                        deLon.text = g.longitude.toFixed(4)
                                        stationCard.suggestZone(g.latitude, g.longitude)
                                        stationCard.autoSave()
                                    }
                                } }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: deLat; label: "Home latitude (e.g. 40.7 or 40:44:55N)"
                                    text: appSettings.configured ? appSettings.deLatitude : ""
                                    field.inputMethodHints: Qt.ImhFormattedNumbersOnly
                                    Connections { target: deLat.field; function onEditingFinished() { stationCard.suggestFromTyped() } } }
                        FormField { id: deLon; label: "Home longitude (e.g. -74 or 74:00:21W)"
                                    text: appSettings.configured ? appSettings.deLongitude : ""
                                    field.inputMethodHints: Qt.ImhFormattedNumbersOnly
                                    Connections { target: deLon.field; function onEditingFinished() { stationCard.suggestFromTyped() } } }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: dxLat; label: "DX latitude (empty = no DX)"
                                    text: appSettings.hasDx ? appSettings.dxLatitude : ""
                                    field.inputMethodHints: Qt.ImhFormattedNumbersOnly }
                        FormField { id: dxLon; label: "DX longitude"
                                    text: appSettings.hasDx ? appSettings.dxLongitude : ""
                                    field.inputMethodHints: Qt.ImhFormattedNumbersOnly }
                    }
                    // A new home location brings its time zone along (the picker can still override it).
                    function suggestZone(lat, lon) {
                        const id = timeZones.suggestZone(lat, lon)
                        zoneHint.text = id.length ? "Time zone set from the location: " + timeZones.labelFor(id) + "."
                                                  : "No time zone known near these coordinates; pick one above."
                        if (id.length) zone.zoneId = id
                    }
                    // Hand-typed home coordinates: suggest once both parse and differ from what is saved.
                    function suggestFromTyped() {
                        const lat = locationTools.parseCoordinate(deLat.text, true)
                        const lon = locationTools.parseCoordinate(deLon.text, false)
                        if (isNaN(lat) || isNaN(lon)) return
                        if (appSettings.configured && lat === appSettings.deLatitude && lon === appSettings.deLongitude) return
                        suggestZone(lat, lon)
                    }
                    // Saved zone, else the zone at the saved home location; the device's zone
                    // only while there is no location yet.
                    TimeZonePicker { id: zone; label: "Time zone"
                                     zoneId: timeZones.stationZone(appSettings.deZone, appSettings.configured ? appSettings.deLatitude : NaN,
                                                                   appSettings.configured ? appSettings.deLongitude : NaN) }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        TimeZonePicker { id: dxZone; label: "DX time zone (empty = automatic)"; Layout.fillWidth: true
                                         Layout.preferredWidth: 0; zoneId: appSettings.dxZoneOverride }
                        AppButton { text: "Auto"; Layout.alignment: Qt.AlignBottom; onClicked: dxZone.zoneId = "" }
                    }
                    Text { id: zoneHint; visible: text.length > 0; color: Theme.secondary
                           font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Text { id: stationError; visible: text.length > 0; color: Theme.bad; font.pixelSize: Theme.fontCaption
                           wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    // Saves as soon as the station is complete; until then it waits quietly
                    // (errors about missing input show when Settings closes).
                    function autoSave() {
                        if (!page.visible) return   // bindings settling while Settings is closed
                        if ([deLat, deLon].some(f => isNaN(coordinate(f, f === deLat)))) return
                        const dxTyped = [dxLat, dxLon].filter(f => f.text.trim().length)
                        if (dxTyped.length === 1 || dxTyped.some(f => isNaN(coordinate(f, f === dxLat)))) return
                        if (save()) stationError.text = ""
                    }
                    Component.onCompleted: { for (const f of [callsign, deLat, deLon, dxLat, dxLon]) f.editingFinished.connect(autoSave) }
                    Connections { target: zone; function onZoneIdChanged() { stationCard.autoSave() } }
                    Connections { target: dxZone; function onZoneIdChanged() { stationCard.autoSave() } }

                    function coordinate(field, latitude) { return locationTools.parseCoordinate(field.text, latitude) }
                    // Saves the station and DX zone when anything changed. DX is optional:
                    // left empty it means no DX target (stored as DX = DE). Returns false with a message
                    // in stationError when the input can't be saved.
                    function save() {
                        const typed = callsign.text.trim().length || deLat.text.trim().length || deLon.text.trim().length
                        if (!appSettings.configured && !typed) return true
                        const de = [coordinate(deLat, true), coordinate(deLon, false)]
                        if (de.some(isNaN)) {
                            stationError.text = "Add your home location: press Find, type a grid square, or enter latitude and longitude."
                            return false
                        }
                        const dxTyped = dxLat.text.trim().length || dxLon.text.trim().length
                        const dx = dxTyped ? [coordinate(dxLat, true), coordinate(dxLon, false)] : de
                        if (dx.some(isNaN)) {
                            stationError.text = "For DX, enter both latitude and longitude, or leave both empty."
                            return false
                        }
                        const changed = !appSettings.configured || callsign.text !== appSettings.callsign
                                        || de[0] !== appSettings.deLatitude || de[1] !== appSettings.deLongitude
                                        || dx[0] !== appSettings.dxLatitude || dx[1] !== appSettings.dxLongitude
                                        || zone.zoneId !== appSettings.deZone
                        if (changed) {
                            stationError.text = appSettings.saveStation(callsign.text, de[0], de[1], dx[0], dx[1], zone.zoneId)
                            if (stationError.text.length) return false
                        }
                        if (dxZone.zoneId !== appSettings.dxZoneOverride) {
                            stationError.text = appSettings.saveDxZone(dxZone.zoneId)
                            if (stationError.text.length) return false
                        }
                        return true
                    }
                }
            }

            AccountsSection { id: accounts; Layout.fillWidth: true }

            DisplaySection { id: displaySection; Layout.fillWidth: true }

            ScreenSection { id: screenSection; Layout.fillWidth: true }

            Card {
                title: "Saved configurations"
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: Theme.secondary; font.pixelSize: Theme.fontCaption
                           text: "Snapshots of these options (not the station or passwords). Pane choices, blanking and alarms apply at once." }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: cfgName; label: "Name"; Layout.fillWidth: true; field.maximumLength: 24 }
                        AppButton { text: "Add snapshot"; Layout.alignment: Qt.AlignBottom
                                    onClicked: { cfgMsg.text = appSettings.saveConfig(cfgName.text) } }
                        AppButton { text: "Rename to"; Layout.alignment: Qt.AlignBottom; visible: cfgPick.current.length > 0
                                    onClicked: { cfgMsg.text = appSettings.renameConfig(cfgPick.current, cfgName.text); if (!cfgMsg.text.length) cfgPick.current = "" } }
                    }
                    Flow {
                        id: cfgPick
                        property string current: ""
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        Repeater {
                            model: appSettings.configNames
                            AppButton { text: modelData; primary: cfgPick.current === modelData
                                        onClicked: { cfgPick.current = modelData; cfgName.text = modelData } }
                        }
                    }
                    RowLayout {
                        visible: cfgPick.current.length > 0
                        spacing: Theme.gap * 0.6
                        AppButton { text: "Restore " + cfgPick.current
                                    onClicked: { cfgMsg.text = appSettings.restoreConfig(cfgPick.current); if (!cfgMsg.text.length) cfgMsg.text = "Restored. Restart to apply alarms." } }
                        AppButton { text: "Delete"; onClicked: { cfgMsg.text = appSettings.deleteConfig(cfgPick.current); if (!cfgMsg.text.length) cfgPick.current = "" } }
                    }
                    Text { id: cfgMsg; color: Theme.bad; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true; visible: text.length > 0 }
                }
            }

            SatelliteAlertsCard { Layout.fillWidth: true }

            Card {
                title: "Demo mode"
                Layout.fillWidth: true
                Flow {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    AppButton {
                        text: appSettings.demoMode ? "Stop demo" : "Start demo"
                        primary: appSettings.demoMode
                        onClicked: appSettings.demoMode = !appSettings.demoMode
                    }
                    Text {
                        text: "Steps every pane through all widgets every 10 s, showing real data. Not saved: it ends when the app restarts, or from a pane's long-press menu."
                        color: Theme.secondary
                        font.pixelSize: Theme.fontCaption
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }
            }

            Card {
                id: spotsCard
                title: "Spots and feeds"
                Layout.fillWidth: true
                ColumnLayout {
                    id: clusterCard
                    width: parent.width
                    readonly property var fields: [udpPort, udpGroup,
                        aprsHost, aprsPort, aprsRadius, meshHost, meshPort, pskMax, pskMatch, adifPath,
                        otaProgram, otaBand, otaMode, otaSort, otaAge, watchList]
                    function changed() { return fields.some(f => f.edited) }
                    // Saves the groups whose fields were typed in (so editing the watch list doesn't
                    // reconnect APRS). Returns false with the message in clusterError on an error.
                    function save() {
                        if (!changed()) return true
                        const steps = [
                            [[watchList], () => appSettings.saveWatchList(watchList.text)],
                            [[adifPath], () => appSettings.saveAdifPath(adifPath.text)],
                            [[otaProgram, otaBand, otaMode, otaAge],
                             () => appSettings.saveOtaFilter(otaProgram.text, otaBand.text, otaMode.text, parseInt(otaAge.text) || 0)],
                            [[otaSort], () => appSettings.saveOtaSort(otaSort.text)],
                            [[aprsHost, aprsPort, aprsRadius],
                             () => appSettings.saveAprs(aprsHost.text, parseInt(aprsPort.text) || 0, parseInt(aprsRadius.text) || 0)],
                            [[meshHost, meshPort], () => appSettings.saveMesh(meshHost.text, parseInt(meshPort.text) || 0)],
                            [[pskMax], () => appSettings.savePskMaxKm(parseInt(pskMax.text) || 0)],
                            [[pskMatch], () => appSettings.savePskMatch(pskMatch.text)],
                            [[udpPort], () => appSettings.saveUdpSpots(parseInt(udpPort.text) || 0, appSettings.udpSpotLan)],
                            [[udpGroup], () => appSettings.saveUdpGroup(udpGroup.text)]
                        ]
                        // "Saved; ..." is a warning (something was trimmed), not a failure.
                        let warning = ""
                        for (const [inputs, step] of steps) {
                            if (!inputs.some(f => f.edited)) continue
                            const message = step()
                            if (message.startsWith("Saved")) { warning = message }
                            else if (message.length) { clusterError.text = message; return false }
                            inputs.forEach(f => f.edited = false)
                        }
                        clusterError.text = warning
                        return true
                    }
                    Component.onCompleted: { for (const f of fields) f.editingFinished.connect(save) }
                    // Switches apply at once; a failure shows under the card.
                    function apply(message) { clusterError.text = message }
                    spacing: Theme.gap * 0.8
                    Text {
                        text: "Choose the DX cluster node under Accounts. These options shape the spots, APRS, Meshtastic and PSKReporter panes."
                        color: Theme.secondary; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                    // WSJT-X / JTDX spots over UDP: two fields side by side, then the switches,
                    // which wrap at their natural width (no label is ever cut on a phone).
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: udpPort; label: "UDP spot input port (0 = off)"; text: appSettings.udpSpotPort
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                        // Multicast needs an Apple entitlement the app doesn't have.
                        FormField { id: udpGroup; label: "Multicast group (blank = off)"; text: appSettings.udpSpotGroup
                                    visible: Qt.platform.os !== "ios" }
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        // iOS always listens on the network (WSJT-X runs on another computer).
                        ToggleChip { id: udpLan; label: "UDP from other computers"; on: appSettings.udpSpotLan
                                    visible: Qt.platform.os !== "ios"
                                    onFlipped: (on) => clusterCard.apply(appSettings.saveUdpSpots(appSettings.udpSpotPort, on)) }
                        ToggleChip { id: udpMe; label: "Only my UDP spots"; on: appSettings.udpSpotByMe
                                    onFlipped: (on) => clusterCard.apply(appSettings.saveUdpScope(on)) }
                        ToggleChip { id: udpSetsDx; label: "UDP spot sets DX (10 min)"; on: appSettings.udpSetsDx
                                    onFlipped: (on) => clusterCard.apply(appSettings.saveUdpSetsDx(on)) }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: aprsHost; label: "APRS-IS server (receive-only)"; text: appSettings.aprsHost; Layout.preferredWidth: 3 }
                        FormField { id: aprsPort; label: "Port"; text: appSettings.aprsPort; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                        FormField { id: aprsRadius; label: "Radius, km"; text: appSettings.aprsRadiusKm; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: meshHost; label: "Meshtastic node on your LAN (receive-only; empty = off)"; text: appSettings.meshHost; Layout.preferredWidth: 3 }
                        FormField { id: meshPort; label: "Port"; text: appSettings.meshPort; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    FormField { id: pskMax; label: "PSKReporter: only receivers within, km of DE (0 = any)"; text: appSettings.pskMaxKm; Layout.fillWidth: true
                                field.inputMethodHints: Qt.ImhDigitsOnly }
                    ToggleChip { id: pskWspr; label: "PSKReporter: include WSPR (wspr.live)"; on: appSettings.pskWspr
                                onFlipped: (on) => clusterCard.apply(appSettings.savePskWspr(on)) }
                    ToggleChip { id: pskRbn; label: "PSKReporter: include RBN skimmers (telnet)"; on: appSettings.pskRbn
                                onFlipped: (on) => clusterCard.apply(appSettings.savePskRbn(on)) }
                    FormField { id: pskMatch; label: "PSKReporter: only receivers starting with (calls/grids, e.g. VK, JA, PM95)"; text: appSettings.pskMatch; Layout.fillWidth: true }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        Text { text: "Show on map:"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; height: 32; verticalAlignment: Text.AlignVCenter }
                        Repeater {
                            model: [["dx", "DX"], ["psk", "PSK"], ["ota", "POTA"], ["hamalert", "HamAlert"], ["aprs", "APRS"], ["mesh", "Mesh"]].concat(Qt.platform.os === "ios" ? [] : [["adif", "Log"]])
                            ToggleChip { label: modelData[1]; on: appSettings.mapSourceVisible(modelData[0])
                                        Connections { target: appSettings; function onOptionsChanged() { on = appSettings.mapSourceVisible(modelData[0]) } }
                                        onFlipped: (on) => appSettings.saveMapSourceVisible(modelData[0], on) }
                        }
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        Text { text: "Map labels:"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; height: 32; verticalAlignment: Text.AlignVCenter }
                        Repeater {
                            model: [["dx", "DX"], ["psk", "PSK"], ["ota", "POTA"], ["hamalert", "HamAlert"], ["aprs", "APRS"], ["mesh", "Mesh"]].concat(Qt.platform.os === "ios" ? [] : [["adif", "Log"]])
                            AppButton { property int mode: appSettings.mapSourceLabel(modelData[0]); checkable: false
                                        text: modelData[1] + ": " + ["auto", "none", "prefix", "call", "dot"][mode + 1]
                                        Connections { target: appSettings; function onOptionsChanged() { mode = appSettings.mapSourceLabel(modelData[0]) } }
                                        onClicked: appSettings.saveMapSourceLabel(modelData[0], mode >= 3 ? -1 : mode + 1) }
                        }
                        AppButton { property int px: appSettings.mapMinLabelPx(); checkable: false
                                    text: "Label gap: " + (px ? px + " px" : "auto")
                                    Connections { target: appSettings; function onOptionsChanged() { px = appSettings.mapMinLabelPx() } }
                                    onClicked: appSettings.saveMapMinLabelPx(px >= 100 ? 0 : px + 20) }
                    }
                    // A typed path can't reach a file inside the iOS sandbox; the log pane is off there too.
                    FormField { id: adifPath; label: "ADIF log file (.adi, read-only; optional)"; text: appSettings.adifPath; Layout.fillWidth: true
                                visible: Qt.platform.os !== "ios" }
                    // Five filters: one row on wide screens, two columns on a phone.
                    GridLayout {
                        Layout.fillWidth: true
                        columns: width < 600 * Theme.scale ? 2 : 5
                        columnSpacing: Theme.gap
                        rowSpacing: Theme.gap * 0.8
                        FormField { id: otaProgram; label: "On the air: POTA/SOTA/WWFF (empty = all)"; text: appSettings.otaProgram; Layout.preferredWidth: 2 }
                        FormField { id: otaBand; label: "Band (20m)"; text: appSettings.otaBand; Layout.preferredWidth: 1 }
                        FormField { id: otaMode; label: "Mode (CW)"; text: appSettings.otaMode; Layout.preferredWidth: 1 }
                        FormField { id: otaSort; label: "Sort (time, freq, dist)"; text: appSettings.otaSort; Layout.preferredWidth: 1 }
                        FormField { id: otaAge; label: "Max age, min (0 = any)"; text: appSettings.otaMaxAgeMin; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    FormField { id: watchList; label: "Watch list (calls, prefixes like VP8*, or @VP8 for a whole DXCC entity, NADXCC, NAPREF, NAGRID or NABAND for spots your ADIF log still needs, OTA/POTA/SOTA/IOTA/WWFF for spots naming a reference; space separated)"; text: appSettings.watchListText; Layout.fillWidth: true }
                    Text { id: clusterError; visible: text.length > 0; color: text.startsWith("Saved") ? Theme.warn : Theme.bad
                           font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
            }

            // Local station hardware. iPhone and iPad have none and do not list these panes
            // (widget_available), so the section is hidden there, like the ADIF path.
            Card {
                id: hardwareSection
                title: "On this computer: station hardware"
                Layout.fillWidth: true
                visible: Qt.platform.os !== "ios"
                ColumnLayout {
                    id: hardwareCard
                    width: parent.width
                    spacing: Theme.gap * 0.8
                    readonly property var fields: [radioHost, radioPort, rotatorHost, rotatorPort, adsbHost, adsbPort,
                        gpsHost, gpsPort, gpsDevice, gpsBaud]
                    function changed() { return fields.some(f => f.edited) }
                    // Returns false with the message in hardwareError when something can't be saved.
                    function save() {
                        if (Qt.platform.os === "ios" || !changed()) return true
                        const steps = [
                            () => appSettings.saveRadio(radioHost.text, parseInt(radioPort.text) || 0),
                            () => appSettings.saveRotator(rotatorHost.text, parseInt(rotatorPort.text) || 0),
                            () => appSettings.saveAdsb(adsbHost.text, parseInt(adsbPort.text) || 0),
                            () => appSettings.saveGps(gpsHost.text, parseInt(gpsPort.text) || 0, gpsDevice.text, parseInt(gpsBaud.text) || 0)
                        ]
                        for (const step of steps) {
                            const message = step()
                            if (message.length) { hardwareError.text = message; return false }
                        }
                        hardwareError.text = ""
                        fields.forEach(f => f.edited = false)
                        return true
                    }
                    Text {
                        text: "Programs that read your radio, rotator, ADS-B receiver and GPS. Read-only: nothing is sent that could change the rig or move the rotator. Use 127.0.0.1 when they run on this computer; changes apply at once."
                        color: Theme.secondary; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: radioHost; label: "Radio: rigctld (Hamlib) address"; text: appSettings.radioHost; Layout.preferredWidth: 3
                                    placeholder: "127.0.0.1"; field.inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase }
                        FormField { id: radioPort; label: "Port"; text: appSettings.radioPort; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: rotatorHost; label: "Rotator: rotctld (Hamlib) address"; text: appSettings.rotatorHost; Layout.preferredWidth: 3
                                    placeholder: "127.0.0.1"; field.inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase }
                        FormField { id: rotatorPort; label: "Port"; text: appSettings.rotatorPort; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: adsbHost; label: "ADS-B: PiAware / dump1090 address"; text: appSettings.adsbHost; Layout.preferredWidth: 3
                                    placeholder: "127.0.0.1"; field.inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase }
                        FormField { id: adsbPort; label: "Port"; text: appSettings.adsbPort; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: gpsHost; label: "GPS: gpsd address"; text: appSettings.gpsHost; Layout.preferredWidth: 3
                                    placeholder: "127.0.0.1"; field.inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase }
                        FormField { id: gpsPort; label: "Port"; text: appSettings.gpsPort; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: gpsDevice; label: "Or a serial GPS instead of gpsd (e.g. /dev/ttyACM0; empty = gpsd)"; text: appSettings.gpsDevice; Layout.preferredWidth: 3
                                    field.inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase }
                        FormField { id: gpsBaud; label: "Baud"; text: appSettings.gpsBaud; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    Text { id: hardwareError; visible: text.length > 0; color: Theme.bad; font.pixelSize: Theme.fontCaption
                           wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Component.onCompleted: { for (const f of fields) f.editingFinished.connect(save) }
                }
            }


            Card {
                title: "Data sources"
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.5
                    Text {
                        text: "HamClock Pro is independent and not affiliated with the providers below. Data is fetched directly from each source."
                        color: Theme.secondary
                        font.pixelSize: Theme.fontCaption
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Repeater {
                        model: dataSources
                        Text {
                            required property var modelData
                            text: modelData.name + ": " + modelData.usedFor + ". " + modelData.terms + " (" + modelData.url + ")"
                            color: Theme.ink
                            font.pixelSize: Theme.fontCaption
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}
