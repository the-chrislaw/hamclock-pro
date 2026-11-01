import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HamClock

// Settings > Accounts: every service that needs a sign-in (or, for the DX cluster, a node),
// one row each with what it is for and whether it works. Tapping a row opens AccountSheet.
Card {
    id: section
    title: "Accounts"

    // Bumped on every account or station change, so the status bindings below re-read the
    // invokable getters (QML cannot see their changes on its own).
    property int revision: 0
    Connections {
        target: appSettings
        function onAccountsChanged() { section.revision++ }
        function onStationChanged() { section.revision++ }
        function onOptionsChanged() { section.revision++ }
    }

    function model(id) { return typeof widgetRegistry !== "undefined" ? widgetRegistry.model(id) : null }
    readonly property var lookupModel: model("lookup")
    readonly property var hamAlertModel: model("hamalert")
    readonly property var hamsatModel: model("hamsat")
    readonly property var clusterModel: model("dxcluster")

    readonly property var services: [
        { id: "hamqth", name: "HamQTH", about: "Callsign lookup: name, QTH and grid. Free." },
        { id: "qrz", name: "QRZ.com", about: "Callsign lookup from QRZ. Needs a paid XML subscription." },
        { id: "hamalert", name: "HamAlert", about: "Your own spot alerts from hamalert.org. Free." },
        { id: "hamsat", name: "hams.at", about: "Satellite activations you can work from here. Free key, optional." },
        { id: "dxcluster", name: "DX cluster", about: "Live DX spots. No account: your callsign and a node." }
    ]

    // Called when Settings opens: check saved lookup sign-ins once per run, so the
    // rows say whether they work instead of only that something is saved.
    function reload() {
        sheet.close()
        if (!lookupModel) return
        for (const id of ["qrz", "hamqth"])
            if (appSettings.accountSignedIn(id) && lookupModel.checks[id] === undefined) lookupModel.checkLogin(id)
    }

    function open(id) { sheet.openFor(id) }

    // {text, tone} for a row; tone is good, warn, bad or quiet.
    function statusFor(id) {
        void revision
        const user = appSettings.accountUser(id)
        if (id === "qrz" || id === "hamqth") {
            if (!appSettings.accountSignedIn(id)) return { text: "Not signed in", tone: "quiet" }
            const c = lookupModel ? lookupModel.checks[id] : undefined
            const state = c ? c.state : ""
            if (state === "checking") return { text: "Checking…", tone: "quiet" }
            if (state === "ok") return { text: "Signed in as " + user + " ✓", tone: "good" }
            if (state === "limited") return { text: "Signed in as " + user + " · no XML subscription", tone: "warn" }
            if (state === "rejected") return { text: "Login rejected", tone: "bad" }
            if (state === "blocked") return { text: "Locked out by QRZ.com for 24 hours", tone: "bad" }
            if (state === "busy") return { text: "Signed in as " + user + " · service busy", tone: "warn" }
            if (state === "unreachable") return { text: "Signed in as " + user + " · can't reach it now", tone: "warn" }
            if (state === "failed") return { text: "Sign-in failed", tone: "bad" }
            return { text: "Signed in as " + user, tone: "good" }
        }
        if (id === "hamalert") {
            if (!appSettings.accountSignedIn(id))
                return user.length ? { text: "Password missing", tone: "warn" } : { text: "Not signed in", tone: "quiet" }
            const s = hamAlertModel ? hamAlertModel.state : ""
            if (s === "live") return { text: "Signed in as " + user + " ✓", tone: "good" }
            if (s === "loading") return { text: "Signing in as " + user + "…", tone: "quiet" }
            if (s === "error") return { text: "Login rejected", tone: "bad" }
            if (s === "offline") return { text: "Signed in as " + user + " · can't reach HamAlert", tone: "warn" }
            return { text: "Signed in as " + user, tone: "good" }
        }
        if (id === "hamsat") {
            const k = hamsatModel ? hamsatModel.keyStatus : (appSettings.hasHamsatKey ? "unknown" : "none")
            if (k === "none") return { text: "No key: public list", tone: "quiet" }
            if (k === "checking") return { text: "Checking key…", tone: "quiet" }
            if (k === "accepted") return { text: "Key accepted ✓", tone: "good" }
            if (k === "rejected") return { text: "Key not accepted", tone: "bad" }
            return { text: "Key saved", tone: "good" }
        }
        // DX cluster
        if (!appSettings.callsign.length) return { text: "Set your callsign under Station", tone: "warn" }
        if (!appSettings.clusterHost.length) return { text: "Choose a node", tone: "quiet" }
        const s = clusterModel ? clusterModel.state : ""
        if (s === "live") return { text: "Connected to " + appSettings.clusterHost + " ✓", tone: "good" }
        if (s === "loading") return { text: "Connecting to " + appSettings.clusterHost + "…", tone: "quiet" }
        if (s === "offline") return { text: clusterModel.statusText.startsWith("Cannot reach") ? "Can't reach " + appSettings.clusterHost
                                                                                             : "Connection lost; retrying", tone: "warn" }
        return { text: appSettings.clusterHost, tone: "quiet" }
    }
    function toneColor(tone) {
        return tone === "good" ? Theme.good : tone === "warn" ? Theme.warn : tone === "bad" ? Theme.bad : Theme.secondary
    }
    function lookupUsesText() {
        void revision
        const s = appSettings.lookupService
        return s === "qrz" ? "Callsign lookups use QRZ.com."
             : s === "hamqth" ? "Callsign lookups use HamQTH."
             : "Callsign lookups are off until you sign in to HamQTH or QRZ.com."
    }

    ColumnLayout {
        width: parent.width
        spacing: Theme.gap * 0.6
        Text {
            text: "Tap a service to sign in or out. Passwords and keys stay on this device, in the Keychain (on the Pi: a file only you can read)."
            color: Theme.secondary; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Repeater {
            model: section.services
            AbstractButton {
                id: row
                required property var modelData
                readonly property var status: section.statusFor(modelData.id)
                Layout.fillWidth: true
                implicitHeight: Math.max(Theme.touch, rowLayout.implicitHeight + Theme.gap * 1.4)
                focusPolicy: Qt.StrongFocus
                Accessible.name: modelData.name + ": " + status.text
                onClicked: section.open(modelData.id)
                Keys.onReturnPressed: section.open(modelData.id)
                Keys.onEnterPressed: section.open(modelData.id)
                background: Rectangle {
                    radius: Theme.radius
                    color: row.down ? Theme.divider : row.hovered ? Theme.surfaceRaised : Theme.surface
                    border.color: row.activeFocus ? Theme.focus : Theme.divider
                    border.width: row.activeFocus ? 2 : 1
                }
                contentItem: Item {}
                RowLayout {
                    id: rowLayout
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter
                              leftMargin: Theme.pad; rightMargin: Theme.pad }
                    spacing: Theme.gap
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2 * Theme.scale
                        Text { text: row.modelData.name; color: Theme.ink; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold
                               Layout.fillWidth: true; elide: Text.ElideRight }
                        Text { text: row.modelData.about; color: Theme.secondary; font.pixelSize: Theme.fontCaption
                               wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        Text { text: row.status.text; color: section.toneColor(row.status.tone); font.pixelSize: Theme.fontCaption
                               font.weight: Font.DemiBold; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    }
                    Text { text: "›"; color: Theme.secondary; font.pixelSize: Theme.fontValue; Layout.alignment: Qt.AlignVCenter }
                }
            }
        }
        Text {
            text: section.lookupUsesText()
            color: Theme.secondary; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
    }

    AccountSheet { id: sheet; accounts: section }
}
