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
                    TimeZonePicker { id: zone; label: "Time zone"
                                     zoneId: appSettings.deZone.length ? appSettings.deZone : timeZones.systemZone() }
                    AppButton { text: "Suggest zone from home coordinates"
                                onClicked: {
                                    const id = timeZones.suggestZone(locationTools.parseCoordinate(deLat.text, true),
                                                                     locationTools.parseCoordinate(deLon.text, false))
                                    zoneHint.text = id.length ? "Suggested: " + timeZones.labelFor(id) + ". Check it before saving."
                                                              : "No suggestion for these coordinates; pick a zone above."
                                    if (id.length) zone.zoneId = id
                                } }
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
                title: "TV overscan"
                Layout.fillWidth: true
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
                        AppButton { id: udpMe; property bool me: appSettings.udpSpotByMe; checkable: false
                                    text: me ? "UDP spots: by me" : "UDP spots: all"; primary: me
                                    onClicked: me = !me; Layout.preferredWidth: 1 }
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
                    FormField { id: pskMax; label: "PSKReporter: only receivers within, km of DE (0 = any)"; text: appSettings.pskMaxKm; Layout.fillWidth: true
                                field.inputMethodHints: Qt.ImhDigitsOnly }
                    FormField { id: adifPath; label: "ADIF log file (.adi, read-only; optional)"; text: appSettings.adifPath; Layout.fillWidth: true }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.gap
                        FormField { id: otaProgram; label: "On the air: POTA/SOTA (empty = both)"; text: appSettings.otaProgram; Layout.preferredWidth: 2 }
                        FormField { id: otaBand; label: "Band (20m)"; text: appSettings.otaBand; Layout.preferredWidth: 1 }
                        FormField { id: otaMode; label: "Mode (CW)"; text: appSettings.otaMode; Layout.preferredWidth: 1 }
                        FormField { id: otaSort; label: "Sort (time, freq, dist)"; text: appSettings.otaSort; Layout.preferredWidth: 1 }
                        FormField { id: otaAge; label: "Max age, min (0 = any)"; text: appSettings.otaMaxAgeMin; Layout.preferredWidth: 1
                                    field.inputMethodHints: Qt.ImhDigitsOnly }
                    }
                    FormField { id: watchList; label: "Watch list (calls, prefixes like VP8*, or @VP8 for a whole DXCC entity, NADXCC, NAPREF or NABAND for spots your ADIF log still needs; space separated)"; text: appSettings.watchListText; Layout.fillWidth: true }
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
                                            clusterError.text = appSettings.savePskMaxKm(parseInt(pskMax.text) || 0)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveUdpSpots(parseInt(udpPort.text) || 0, udpLan.lan)
                                        if (clusterError.text === "")
                                            clusterError.text = appSettings.saveUdpScope(udpMe.me)
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
