import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HamClock

// One account from Settings > Accounts: what it is, what it costs, where to sign up, the
// login fields, and Sign in (saves to the Keychain and checks it at once) / Sign out.
Popup {
    id: sheet
    property var accounts: null       // the AccountsSection: status text and widget models
    property string service: "hamqth"
    readonly property var info: details[service] || details.hamqth
    readonly property bool isCluster: service === "dxcluster"
    readonly property bool hasUser: service === "qrz" || service === "hamqth" || service === "hamalert"
    readonly property var status: accounts ? accounts.statusFor(service) : ({ text: "", tone: "quiet" })
    readonly property bool signedIn: {
        void (accounts ? accounts.revision : 0)
        return isCluster ? appSettings.clusterHost.length > 0 : appSettings.accountSignedIn(service)
    }

    readonly property var details: ({
        hamqth: { name: "HamQTH", cost: "Free account",
                  about: "Looks up names, locations and grids for callsigns.",
                  how: "Register at hamqth.com. Your user name is your callsign; use your HamQTH password.",
                  userLabel: "HamQTH user name (your callsign)", secretLabel: "HamQTH password",
                  signUp: "https://www.hamqth.com/register", signUpText: "Create a HamQTH account" },
        qrz: { name: "QRZ.com", cost: "Paid: XML Logbook Data subscription (or Premium / Platinum)",
               about: "Looks up callsigns in QRZ.com. A free QRZ account signs in but gets only part of each record.",
               how: "Use your QRZ.com user name (usually your callsign) and your QRZ.com password. When both QRZ.com and HamQTH are signed in, lookups use QRZ.com.",
               userLabel: "QRZ.com user name", secretLabel: "QRZ.com password",
               signUp: "https://shop.qrz.com/collections/subscriptions", signUpText: "Get a QRZ.com XML subscription" },
        hamalert: { name: "HamAlert", cost: "Free account",
                    about: "Spots that match your HamAlert triggers, live (telnet to hamalert.org, port 7300).",
                    how: "On hamalert.org, add triggers and tick their Telnet action, or no spots arrive. Use your HamAlert user name and password (no spaces).",
                    userLabel: "HamAlert user name", secretLabel: "HamAlert password",
                    signUp: "https://hamalert.org/register", signUpText: "Create a HamAlert account" },
        hamsat: { name: "hams.at", cost: "Free; the key is optional",
                  about: "Without a key the hams.at pane shows the public list. With your key, hams.at says which passes you can work from your station.",
                  how: "Sign in on hams.at, set your grid there, open Settings on hams.at and copy the API key.",
                  userLabel: "", secretLabel: "hams.at API key",
                  signUp: "https://hams.at/users/register", signUpText: "Create a hams.at account" },
        dxcluster: { name: "DX cluster", cost: "Free, no account",
                     about: "Live DX spots for the DX cluster, Band activity, Beacons and Watch list panes.",
                     how: "Pick a public node (or type your own). Nodes log you in with your callsign and are for licensed amateurs using their real call.",
                     userLabel: "", secretLabel: "", signUp: "", signUpText: "" }
    })

    function openFor(id) {
        service = id
        message.text = ""
        user.text = appSettings.accountUser(id)
        secret.text = ""
        if (id === "dxcluster") {
            host.text = appSettings.clusterHost
            port.text = appSettings.clusterPort
            ssid.text = appSettings.clusterSsid
            commands.text = appSettings.clusterCommandsText.split("\n").join("; ")
        }
        open()
    }

    function signIn() {
        let error = ""
        if (isCluster) {
            error = appSettings.saveCluster(host.text, parseInt(port.text) || 0)
            if (!error.length || error.startsWith("Saved"))
                error = appSettings.saveClusterLogin(parseInt(ssid.text) || 0, commands.text.split(";").join("\n"))
        } else {
            error = appSettings.signIn(service, user.text, secret.text)
        }
        secret.text = ""
        message.text = error
        if (error.length && !error.startsWith("Saved")) return
        // HamAlert, hams.at and the cluster reconnect on their own when their login changes.
        if ((service === "qrz" || service === "hamqth") && accounts && accounts.lookupModel)
            accounts.lookupModel.checkLogin(service)
    }

    function signOut() {
        message.text = isCluster ? appSettings.saveCluster("", appSettings.clusterPort) : appSettings.signOut(service)
        user.text = ""
        secret.text = ""
        host.text = ""
    }

    parent: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: Theme.pad
    readonly property real roomW: parent ? parent.width : 640
    readonly property real roomH: parent ? parent.height : 480
    width: Math.min(roomW - 2 * Theme.pad, 520 * Theme.scale)
    height: Math.min(roomH - 2 * Theme.pad, body.implicitHeight + 2 * padding)
    x: (roomW - width) / 2
    y: Math.max(Theme.pad, (roomH - height) / 3)
    background: Rectangle { radius: Theme.radius; color: Theme.surfaceRaised; border.color: Theme.divider }
    onOpened: (hasUser ? user.field : isCluster ? host.field : secret.field).forceActiveFocus()

    contentItem: Flickable {
        clip: true
        contentHeight: body.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        ScrollIndicator.vertical: ScrollIndicator {}
        ColumnLayout {
            id: body
            width: parent.width
            spacing: Theme.gap * 0.8

            RowLayout {
                Layout.fillWidth: true
                Text { text: sheet.info.name; color: Theme.ink; font.pixelSize: Theme.fontTitle * 1.2; font.weight: Font.DemiBold
                       Layout.fillWidth: true; elide: Text.ElideRight }
                AppButton { text: "Close"; onClicked: sheet.close() }
            }
            Text { text: sheet.info.cost; color: Theme.ink; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
                   wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Text { text: sheet.info.about; color: Theme.secondary; font.pixelSize: Theme.fontCaption
                   wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Text { text: sheet.info.how; color: Theme.secondary; font.pixelSize: Theme.fontCaption
                   wrapMode: Text.WordWrap; Layout.fillWidth: true }
            AppButton {
                visible: sheet.info.signUp.length > 0
                text: sheet.info.signUpText
                Layout.fillWidth: true
                onClicked: Qt.openUrlExternally(sheet.info.signUp)
            }

            // Status, live: changes as the check or the connection progresses.
            Text {
                text: sheet.status.text
                color: sheet.accounts ? sheet.accounts.toneColor(sheet.status.tone) : Theme.secondary
                font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
                wrapMode: Text.WordWrap; Layout.fillWidth: true
            }
            Text {
                readonly property var check: sheet.accounts && sheet.accounts.lookupModel && !sheet.isCluster
                                             ? sheet.accounts.lookupModel.checks[sheet.service] : undefined
                visible: text.length > 0
                text: check && check.state !== "ok" && check.state !== "checking" ? check.message : ""
                color: Theme.secondary; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true
            }

            FormField { id: user; visible: sheet.hasUser; label: sheet.info.userLabel
                        field.inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase }
            FormField { id: secret; visible: !sheet.isCluster; label: sheet.info.secretLabel
                        echoMode: TextInput.Password
                        field.inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhSensitiveData
                        placeholder: sheet.signedIn ? "Saved — leave empty to keep" : ""
}

            // DX cluster: suggested public nodes fill in the host and port.
            Text { visible: sheet.isCluster; text: "Suggested public nodes"; color: Theme.secondary
                   font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
            Flow {
                visible: sheet.isCluster
                Layout.fillWidth: true
                spacing: Theme.gap * 0.6
                Repeater {
                    model: sheet.isCluster ? appSettings.clusterSuggestions() : []
                    AppButton {
                        required property var modelData
                        text: modelData.label
                        primary: host.text === modelData.host && parseInt(port.text) === modelData.port
                        Accessible.description: modelData.host + ", " + modelData.note
                        onClicked: { host.text = modelData.host; port.text = modelData.port }
                    }
                }
            }
            Text {
                visible: sheet.isCluster
                text: appSettings.clusterSuggestions().map(n => n.label + ": " + n.note).join(" · ")
                color: Theme.secondary; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true
            }
            RowLayout {
                visible: sheet.isCluster
                Layout.fillWidth: true
                spacing: Theme.gap
                FormField { id: host; label: "Node host name"; Layout.preferredWidth: 3
                            field.inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly }
                FormField { id: port; label: "Port"; Layout.preferredWidth: 1; field.inputMethodHints: Qt.ImhDigitsOnly }
            }
            RowLayout {
                visible: sheet.isCluster
                Layout.fillWidth: true
                spacing: Theme.gap
                FormField { id: ssid; label: "SSID (0 = none)"; Layout.preferredWidth: 1; field.inputMethodHints: Qt.ImhDigitsOnly }
                FormField { id: commands; label: "Commands after login (separate with ;)"; Layout.preferredWidth: 3 }
            }

            Text { id: message; visible: text.length > 0; color: text.startsWith("Saved") ? Theme.warn : Theme.bad
                   font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true }

            // Flow: the buttons wrap on a phone instead of being cut.
            Flow {
                Layout.fillWidth: true
                spacing: Theme.gap * 0.6
                layoutDirection: Qt.RightToLeft
                AppButton { text: sheet.isCluster ? "Connect" : sheet.service === "hamsat" ? "Use key" : "Sign in"
                            primary: true; onClicked: sheet.signIn() }
                AppButton { visible: sheet.signedIn
                            text: sheet.isCluster ? "Disconnect" : sheet.service === "hamsat" ? "Remove key" : "Sign out"
                            onClicked: sheet.signOut() }
            }
        }
    }
}
