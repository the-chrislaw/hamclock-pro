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

        ColumnLayout {
            id: column
            x: Theme.pad
            y: Theme.pad
            width: Math.min(flick.width - 2 * Theme.pad, 640 * Theme.scale)
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
                        FormField { id: deLat; label: "Home latitude (°N, negative = S)"
                                    text: appSettings.configured ? appSettings.deLatitude : ""
                                    field.inputMethodHints: Qt.ImhFormattedNumbersOnly }
                        FormField { id: deLon; label: "Home longitude (°E, negative = W)"
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
                    FormField { id: zone; label: "Time zone (IANA, e.g. America/Los_Angeles)"; text: appSettings.deZone }
                    RowLayout {
                        Text { id: stationError; color: Theme.bad; font.pixelSize: Theme.fontCaption
                               wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        AppButton {
                            text: "Save station"
                            primary: true
                            onClicked: {
                                const values = [num(deLat.text), num(deLon.text), num(dxLat.text), num(dxLon.text)]
                                if (values.some(isNaN)) { stationError.text = "Enter all four coordinates as decimal degrees."; return }
                                stationError.text = appSettings.saveStation(callsign.text, values[0], values[1],
                                                                            values[2], values[3], zone.text)
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
                        Text { id: clusterError; color: Theme.bad; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        AppButton { text: "Save cluster"; primary: true
                                    onClicked: clusterError.text = appSettings.saveCluster(clusterHost.text, parseInt(clusterPort.text) || 0) }
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
