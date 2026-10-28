import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HamClock

// Settings: station, appearance, DX cluster and callsign-lookup account.
// Each section saves on its own and shows the validation error from C++.
Rectangle {
    id: page
    color: Theme.canvas
    signal closeRequested()

    function num(text) { const v = Number(text.trim()); return text.trim().length && isFinite(v) ? v : NaN }

    Flickable {
        id: flick
        anchors.fill: parent
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
            y: Theme.pad
            spacing: Theme.gap

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Settings"
                    color: Theme.ink
                    font.pixelSize: Theme.fontValue
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }
                AppButton { text: "Done"; primary: true; onClicked: page.closeRequested() }
            }

            Card {
                title: "Station"
                Layout.fillWidth: true
                ColumnLayout {
                    id: stationCard
                    width: parent.width
                    spacing: Theme.gap * 0.8
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
                                    }
                                } }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: deLat; label: "Home latitude (e.g. 40.7 or 40:44:55N)"
                                    text: appSettings.configured ? appSettings.deLatitude : ""
                                    field.inputMethodHints: Qt.ImhFormattedNumbersOnly }
                        FormField { id: deLon; label: "Home longitude (e.g. -74 or 74:00:21W)"
                                    text: appSettings.configured ? appSettings.deLongitude : ""
                                    field.inputMethodHints: Qt.ImhFormattedNumbersOnly }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: dxLat; label: "DX latitude"
                                    text: appSettings.configured ? appSettings.dxLatitude : ""
                                    field.inputMethodHints: Qt.ImhFormattedNumbersOnly }
                        FormField { id: dxLon; label: "DX longitude"
                                    text: appSettings.configured ? appSettings.dxLongitude : ""
                                    field.inputMethodHints: Qt.ImhFormattedNumbersOnly }
                    }
                    function suggestZone(lat, lon) {
                        const id = timeZones.suggestZone(lat, lon)
                        zoneHint.text = id.length ? "Suggested: " + timeZones.labelFor(id) + ". Check it before saving."
                                                  : "No zone suggestion for these coordinates; pick one above."
                        if (id.length) zone.zoneId = id
                    }
                    TimeZonePicker { id: zone; label: "Time zone"
                                     zoneId: appSettings.deZone.length ? appSettings.deZone : timeZones.systemZone() }
                    AppButton { text: "Suggest zone from home coordinates"
                                onClicked: {
                                    stationCard.suggestZone(locationTools.parseCoordinate(deLat.text, true),
                                                locationTools.parseCoordinate(deLon.text, false))
                                } }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        TimeZonePicker { id: dxZone; label: "DX time zone (empty = automatic)"; Layout.fillWidth: true
                                         zoneId: appSettings.dxZoneOverride }
                        AppButton { text: "Save"; Layout.alignment: Qt.AlignBottom; onClicked: appSettings.saveDxZone(dxZone.zoneId) }
                        AppButton { text: "Auto"; Layout.alignment: Qt.AlignBottom
                                    onClicked: { dxZone.zoneId = ""; appSettings.saveDxZone("") } }
                    }
                    Text { id: zoneHint; visible: text.length > 0; color: Theme.secondary
                           font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        Text { id: stationError; color: Theme.bad; font.pixelSize: Theme.fontCaption
                               wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        AppButton {
                            text: "Save station"
                            primary: true
                            onClicked: {
                                const values = [locationTools.parseCoordinate(deLat.text, true), locationTools.parseCoordinate(deLon.text, false),
                                                locationTools.parseCoordinate(dxLat.text, true), locationTools.parseCoordinate(dxLon.text, false)]
                                if (values.some(isNaN)) { stationError.text = "Enter all four coordinates as decimal degrees or degrees:minutes:seconds (N/S, E/W)."; return }
                                stationError.text = appSettings.saveStation(callsign.text, values[0], values[1],
                                                                            values[2], values[3], zone.zoneId)
                            }
                        }
                    }
                }
            }

            Card {
                title: "Appearance"
                Layout.fillWidth: true
                RowLayout {
                    spacing: Theme.gap * 0.6
                    Repeater {
                        model: [["system", "System"], ["light", "Light"], ["dark", "Dark"]]
                        AppButton {
                            text: modelData[1]
                            primary: appSettings.appearance === modelData[0]
                            onClicked: appSettings.appearance = modelData[0]
                        }
                    }
                }
            }

            Card {
                title: "Distance units"
                Layout.fillWidth: true
                RowLayout {
                    spacing: Theme.gap * 0.6
                    Repeater {
                        model: [[false, "Metric (km)"], [true, "Imperial (miles)"]]
                        AppButton {
                            text: modelData[1]
                            primary: appSettings.imperialUnits === modelData[0]
                            onClicked: appSettings.saveImperialUnits(modelData[0])
                        }
                    }
                }
            }

            Card {
                title: "Clock format"
                Layout.fillWidth: true
                Flow {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    Repeater {
                        model: [[0, "Thu 15 Oct"], [1, "Thu Oct 15"], [2, "2026-10-15"]]
                        AppButton {
                            text: modelData[1]
                            primary: appSettings.dateOrder === modelData[0]
                            onClicked: appSettings.saveClockFormat(modelData[0], appSettings.clock12h)
                        }
                    }
                    Repeater {
                        model: [[false, "24 h"], [true, "12 h AM/PM"]]
                        AppButton {
                            text: modelData[1]
                            primary: appSettings.clock12h === modelData[0]
                            onClicked: appSettings.saveClockFormat(appSettings.dateOrder, modelData[0])
                        }
                    }
                    Repeater {
                        model: [[false, "Week starts Mon"], [true, "Week starts Sun"]]
                        AppButton {
                            text: modelData[1]
                            primary: appSettings.weekStartsSunday === modelData[0]
                            onClicked: appSettings.saveWeekStart(modelData[0])
                        }
                    }
                }
            }

            Card {
                title: "Callsign banner"
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    AppButton { id: pskWspr; property bool on: appSettings.pskWspr; checkable: false
                                text: on ? "PSKReporter card: include WSPR (wspr.live) on" : "PSKReporter card: include WSPR (wspr.live) off"; primary: on
                                onClicked: { on = !on; clusterError.text = appSettings.savePskWspr(on) } }
                    AppButton { id: pskRbn; property bool on: appSettings.pskRbn; checkable: false
                                text: on ? "PSKReporter card: include RBN skimmers (telnet) on" : "PSKReporter card: include RBN skimmers (telnet) off"; primary: on
                                onClicked: { on = !on; clusterError.text = appSettings.savePskRbn(on) } }
                    FormField { id: pskMatch; label: "PSKReporter: only receivers starting with (calls/grids, e.g. VK, JA, PM95)"; text: appSettings.pskMatch; Layout.fillWidth: true }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        Repeater {
                            model: ["Theme", "Amber", "Green", "Red", "White", "Rainbow"]
                            AppButton {
                                text: modelData
                                primary: appSettings.bannerStyle === index
                                onClicked: appSettings.saveBannerStyle(index)
                            }
                        }
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        Repeater {
                            model: ["No fill", "Black", "Navy", "Maroon", "White"]
                            AppButton {
                                text: "Back: " + modelData
                                primary: appSettings.bannerBackground === index
                                onClicked: appSettings.saveBannerBackground(index)
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: bannerTitle; label: "Custom title (tap the banner to show it; empty = off)"; text: appSettings.bannerTitle
                            field.maximumLength: 24 }
                        AppButton { text: "Save"; Layout.alignment: Qt.AlignBottom; onClicked: appSettings.saveBannerTitle(bannerTitle.text) }
                    }
                }
            }

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
                        AppButton { text: "Save"; Layout.alignment: Qt.AlignBottom
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

            Card {
                title: "Text size"
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    Text {
                        text: "Larger type for TVs viewed from a distance. Panes show less at once."
                        color: Theme.secondary
                        font.pixelSize: Theme.fontCaption
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        Repeater {
                            model: [100, 115, 130, 150]
                            AppButton {
                                text: modelData === 100 ? "Normal" : modelData + " %"
                                primary: appSettings.textScalePercent === modelData
                                onClicked: appSettings.saveTextScale(modelData)
                            }
                        }
                    }
                }
            }

            Card {
                title: "Map size"
                visible: !Theme.phone   // phones show one swipeable pane
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    Text {
                        text: "Panes fill the width of the screen. A smaller map leaves room for a second row of panes."
                        color: Theme.secondary
                        font.pixelSize: Theme.fontCaption
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        Repeater {
                            model: [{ id: "large", label: "Large" }, { id: "medium", label: "Medium" }, { id: "small", label: "Small" }]
                            AppButton {
                                text: modelData.label
                                primary: appSettings.mapSize === modelData.id
                                onClicked: appSettings.saveMapSize(modelData.id)
                            }
                        }
                    }
                }
            }

            Card {
                title: "Accent colour"
                Layout.fillWidth: true
                Flow {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    Repeater {
                        model: ["Blue", "Green", "Amber", "Red", "Purple"]
                        AppButton {
                            text: modelData
                            primary: appSettings.accentStyle === index
                            onClicked: appSettings.saveAccentStyle(index)
                        }
                    }
                }
            }

            Card {
                title: "Big Clock face"
                Layout.fillWidth: true
                Flow {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    Repeater {
                        model: ["Digital", "Analog"]
                        AppButton {
                            text: modelData
                            primary: appSettings.bigClockFace === index
                            onClicked: appSettings.saveBigClockFace(index)
                        }
                    }
                }
            }

            Card {
                title: "TV overscan"
                Layout.fillWidth: true
                visible: Qt.platform.os !== "ios" // kiosk/TV setting, not for iPhone/iPad
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    Text {
                        text: "Inset kept free on every edge, for TVs that crop the picture."
                        color: Theme.secondary
                        font.pixelSize: Theme.fontCaption
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        Repeater {
                            model: [0, 2, 4, 6, 8, 10]
                            AppButton {
                                text: modelData === 0 ? "Off" : modelData + " %"
                                primary: appSettings.overscanPercent === modelData
                                onClicked: appSettings.saveOverscan(modelData)
                            }
                        }
                    }
                }
            }

            Card {
                title: "Screen blanking"
                Layout.fillWidth: true
                visible: Qt.platform.os !== "ios" // kiosk/TV setting, not for iPhone/iPad
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    Text {
                        text: "Black overlay for kiosks (the backlight stays on). Tap to wake; inside the night window it blanks again after 1 minute."
                        color: Theme.secondary
                        font.pixelSize: Theme.fontCaption
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        Repeater {
                            model: [0, 5, 15, 30, 60]
                            AppButton {
                                text: modelData === 0 ? "Idle: off" : "Idle " + modelData + " min"
                                primary: appSettings.blankIdleMinutes === modelData
                                onClicked: appSettings.saveBlanking(modelData, appSettings.blankOffHour, appSettings.blankOnHour)
                            }
                        }
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        Repeater {
                            model: [[-1, -1, "Night: off"], [22, 6, "22:00-06:00"], [23, 7, "23:00-07:00"], [0, 6, "00:00-06:00"]]
                            AppButton {
                                text: modelData[2]
                                primary: appSettings.blankOffHour === modelData[0] && appSettings.blankOnHour === modelData[1]
                                onClicked: appSettings.saveBlanking(appSettings.blankIdleMinutes, modelData[0], modelData[1])
                            }
                        }
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        visible: appSettings.blankOffHour >= 0
                        Repeater {
                            model: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
                            AppButton {
                                text: modelData
                                primary: (appSettings.blankDays >> index) & 1
                                onClicked: appSettings.saveBlankDays(appSettings.blankDays ^ (1 << index))
                            }
                        }
                    }
                }
            }

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
                id: exitCard
                title: "Exit"
                Layout.fillWidth: true
                visible: Qt.platform.os !== "ios"
                property bool armed: false
                Timer { id: exitDisarm; interval: 5000; onTriggered: exitCard.armed = false }
                Flow {
                    width: parent.width
                    spacing: Theme.gap * 0.6
                    AppButton {
                        text: exitCard.armed ? "Tap again to exit" : "Exit HamClock Pro"
                        primary: exitCard.armed
                        onClicked: {
                            if (exitCard.armed) { Qt.quit(); return }
                            exitCard.armed = true
                            exitDisarm.restart()
                        }
                    }
                    Text {
                        text: "Closes the app only; it does not reboot or power off the device."
                        color: Theme.secondary
                        font.pixelSize: Theme.fontCaption
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }
            }

            Card {
                title: "DX cluster"
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.8
                    Text {
                        text: "Spots come from a DX cluster node you choose; it logs in with your callsign. Leave empty to stay disconnected."
                        color: Theme.secondary; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: clusterHost; label: "Node host name"; text: appSettings.clusterHost; Layout.preferredWidth: 3 }
                        FormField { id: clusterPort; label: "Port"; text: appSettings.clusterPort; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: clusterSsid; label: "SSID (0 = none)"; text: appSettings.clusterSsid; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                        FormField { id: clusterCmds; label: "Commands after login (separate with ;)"; text: appSettings.clusterCommandsText.split("\n").join("; "); Layout.preferredWidth: 3 }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: udpPort; label: "UDP spot input port (0 = off)"; text: appSettings.udpSpotPort; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                        AppButton { id: udpLan; property bool lan: appSettings.udpSpotLan; checkable: false
                                    text: lan ? "UDP from LAN: on" : "UDP from this computer only"; primary: lan
                                    onClicked: lan = !lan; Layout.preferredWidth: 2 }
                        FormField { id: udpGroup; label: "Multicast group (blank = off)"; text: appSettings.udpSpotGroup; Layout.preferredWidth: 1 }
                        AppButton { id: udpMe; property bool me: appSettings.udpSpotByMe; checkable: false
                                    text: me ? "UDP spots: by me" : "UDP spots: all"; primary: me
                                    onClicked: me = !me; Layout.preferredWidth: 1 }
                        AppButton { id: udpSetsDx; property bool on: appSettings.udpSetsDx; checkable: false
                                    text: on ? "UDP sets DX: on (10 min)" : "UDP sets DX: off"; primary: on
                                    onClicked: on = !on; Layout.preferredWidth: 1 }
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
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        Text { text: "Show on map:"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; height: 32; verticalAlignment: Text.AlignVCenter }
                        Repeater {
                            model: [["dx", "DX"], ["psk", "PSK"], ["ota", "POTA"], ["hamalert", "HamAlert"], ["aprs", "APRS"], ["mesh", "Mesh"], ["adif", "Log"]]
                            AppButton { property bool on: appSettings.mapSourceVisible(modelData[0]); checkable: false
                                        text: modelData[1] + (on ? ": on" : ": off"); primary: on
                                        Connections { target: appSettings; function onOptionsChanged() { on = appSettings.mapSourceVisible(modelData[0]) } }
                                        onClicked: appSettings.saveMapSourceVisible(modelData[0], !on) }
                        }
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        Text { text: "Map labels:"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; height: 32; verticalAlignment: Text.AlignVCenter }
                        Repeater {
                            model: [["dx", "DX"], ["psk", "PSK"], ["ota", "POTA"], ["hamalert", "HamAlert"], ["aprs", "APRS"], ["mesh", "Mesh"], ["adif", "Log"]]
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
                    FormField { id: adifPath; label: "ADIF log file (.adi, read-only; optional)"; text: appSettings.adifPath; Layout.fillWidth: true }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: otaProgram; label: "On the air: POTA/SOTA/WWFF (empty = all)"; text: appSettings.otaProgram; Layout.preferredWidth: 2 }
                        FormField { id: otaBand; label: "Band (20m)"; text: appSettings.otaBand; Layout.preferredWidth: 1 }
                        FormField { id: otaMode; label: "Mode (CW)"; text: appSettings.otaMode; Layout.preferredWidth: 1 }
                        FormField { id: otaSort; label: "Sort (time, freq, dist)"; text: appSettings.otaSort; Layout.preferredWidth: 1 }
                        FormField { id: otaAge; label: "Max age, min (0 = any)"; text: appSettings.otaMaxAgeMin; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    FormField { id: watchList; label: "Watch list (calls, prefixes like VP8*, or @VP8 for a whole DXCC entity, NADXCC, NAPREF, NAGRID or NABAND for spots your ADIF log still needs, OTA/POTA/SOTA/IOTA/WWFF for spots naming a reference; space separated)"; text: appSettings.watchListText; Layout.fillWidth: true }
                    RowLayout {
                        Text { id: clusterError; color: Theme.bad; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        AppButton { text: "Save cluster"; primary: true
                                    onClicked: {
                                        clusterError.text = appSettings.saveCluster(clusterHost.text, parseInt(clusterPort.text) || 0)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveClusterLogin(parseInt(clusterSsid.text) || 0, clusterCmds.text.split(";").join("\n"))
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveWatchList(watchList.text)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveAdifPath(adifPath.text)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveOtaFilter(otaProgram.text, otaBand.text, otaMode.text, parseInt(otaAge.text) || 0)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveOtaSort(otaSort.text)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveAprs(aprsHost.text, parseInt(aprsPort.text) || 0, parseInt(aprsRadius.text) || 0)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveMesh(meshHost.text, parseInt(meshPort.text) || 0)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.savePskMaxKm(parseInt(pskMax.text) || 0)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.savePskMatch(pskMatch.text)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveUdpSpots(parseInt(udpPort.text) || 0, udpLan.lan)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveUdpGroup(udpGroup.text)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveUdpScope(udpMe.me)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveUdpSetsDx(udpSetsDx.on)
                                    } }
                    }
                }
            }

            Card {
                title: "HamAlert"
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.8
                    Text {
                        text: "Optional. Personal spot alerts from hamalert.org (telnet). Stored only on this device, readable by you alone. Leave the user name empty to turn it off."
                        color: Theme.secondary; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                    FormField { id: hamAlertUser; label: "HamAlert user name"; text: appSettings.hamAlertUser; Layout.fillWidth: true }
                    FormField { id: hamAlertPassword; label: "Password"; echoMode: TextInput.Password; Layout.fillWidth: true
                                placeholder: appSettings.hasHamAlertPassword ? "Saved — leave empty to keep" : "" }
                    RowLayout {
                        Text { id: hamAlertError; color: Theme.bad; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        AppButton { text: "Save HamAlert"; primary: true
                                    onClicked: {
                                        hamAlertError.text = appSettings.saveHamAlert(hamAlertUser.text, hamAlertPassword.text)
                                        hamAlertPassword.text = ""
                                    } }
                    }
                }
            }

            Card {
                title: "hams.at API key"
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.8
                    Text {
                        text: "Optional. Copy the key from your hams.at Settings page to see which satellite activations are workable from your station. Stored only on this device, readable by you alone. Save an empty key to remove it."
                        color: Theme.secondary; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                    FormField { id: hamsatKey; label: "API key"; echoMode: TextInput.Password; Layout.fillWidth: true
                                placeholder: appSettings.hasHamsatKey ? "Saved — save empty to remove" : "" }
                    RowLayout {
                        Text { id: hamsatKeyError; color: Theme.bad; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        AppButton { text: "Save key"; primary: true
                                    onClicked: {
                                        hamsatKeyError.text = appSettings.saveHamsatKey(hamsatKey.text)
                                        hamsatKey.text = ""
                                    } }
                    }
                }
            }

            Card {
                title: "Callsign lookup account"
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.gap * 0.8
                    Text {
                        text: "Optional. Used to look up names and locations for spotted callsigns. QRZ.com needs an XML subscription; HamQTH is free. Stored only on this device, readable by you alone."
                        color: Theme.secondary; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true
                    }
                    property string service: appSettings.lookupService
                    RowLayout {
                        spacing: Theme.gap * 0.6
                        Repeater {
                            model: [["none", "None"], ["qrz", "QRZ.com"], ["hamqth", "HamQTH"]]
                            AppButton {
                                text: modelData[1]
                                primary: parent.parent.service === modelData[0]
                                onClicked: parent.parent.service = modelData[0]
                            }
                        }
                    }
                    FormField { id: lookupUser; visible: parent.service !== "none"; label: "User name"; text: appSettings.lookupUser }
                    FormField { id: lookupPassword; visible: parent.service !== "none"; label: "Password"
                                echoMode: TextInput.Password
                                placeholder: appSettings.hasLookupPassword ? "Saved — leave empty to keep" : "" }
                    RowLayout {
                        Text { id: accountError; color: Theme.bad; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        AppButton {
                            text: "Save account"
                            primary: true
                            onClicked: {
                                accountError.text = appSettings.saveAccount(parent.parent.service, lookupUser.text, lookupPassword.text)
                                lookupPassword.text = ""
                            }
                        }
                    }
                }
            }
        }
    }
}
