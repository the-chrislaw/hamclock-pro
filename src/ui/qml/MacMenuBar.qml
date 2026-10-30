import QtQuick
import QtQuick.Window
import Qt.labs.platform as Platform

// Native macOS menu bar: About and Settings in the app menu (beside the system's
// Services, Hide and Quit), Edit, View, Window and Help. Main.qml creates it on the
// Mac only, so the Pi kiosk, Linux and iOS neither show a menu bar nor need
// Qt.labs.platform. The page state comes in through properties and the requests go
// out as signals, so the menus can be checked without the rest of the app.
QtObject {
    id: root

    required property Window appWindow
    property string version: Qt.application.version
    property string supportUrl: "https://the-chrislaw.github.io/hamclock-pro/support/"
    property bool settingsAvailable: true
    property bool pathAvailable: false
    property bool showPath: false
    property string appearance: "system"

    signal settingsRequested()
    signal pathToggleRequested()
    signal appearanceRequested(string appearance)


    // Edit commands go to the focused text field: the native menu takes the key
    // equivalents (Command-C and so on) before the field sees them.
    function edit(command) {
        const item = appWindow.activeFocusItem
        if (item && typeof item[command] === "function") item[command]()
    }

    // Checkable items break their checked binding when toggled, so the radio state is
    // set from the saved appearance instead (also when Settings changes it).
    function syncAppearance() {
        lightItem.checked = appearance === "light"
        darkItem.checked = appearance === "dark"
        systemItem.checked = appearance !== "light" && appearance !== "dark"
    }
    onAppearanceChanged: syncAppearance()
    Component.onCompleted: {
        syncAppearance()
        // The menu bar finds the app window through its parents and would then wait
        // until that window next gains focus before Qt installs it. Detached from the
        // window it is the global menu bar, installed at once (the app has one window).
        bar.window = null
    }

    readonly property Platform.MessageDialog aboutDialog: Platform.MessageDialog {
        id: about
        title: "About HamClock Pro"
        text: "HamClock Pro " + root.version
        informativeText: "Amateur-radio dashboard by Christopher Law (KM7IBM). "
            + "Free and open source; not affiliated with the original HamClock or its author."
        buttons: Platform.MessageDialog.Ok
    }

    readonly property Platform.MenuBar menuBar: Platform.MenuBar {
        id: bar

        Platform.Menu {
            objectName: "editMenu"
            title: "Edit"
            Platform.MenuItem { text: "Undo"; shortcut: StandardKey.Undo; role: Platform.MenuItem.NoRole; onTriggered: root.edit("undo") }
            Platform.MenuItem { text: "Redo"; shortcut: StandardKey.Redo; role: Platform.MenuItem.NoRole; onTriggered: root.edit("redo") }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Cut"; shortcut: StandardKey.Cut; role: Platform.MenuItem.NoRole; onTriggered: root.edit("cut") }
            Platform.MenuItem { text: "Copy"; shortcut: StandardKey.Copy; role: Platform.MenuItem.NoRole; onTriggered: root.edit("copy") }
            Platform.MenuItem { text: "Paste"; shortcut: StandardKey.Paste; role: Platform.MenuItem.NoRole; onTriggered: root.edit("paste") }
            Platform.MenuItem { text: "Select All"; shortcut: StandardKey.SelectAll; role: Platform.MenuItem.NoRole; onTriggered: root.edit("selectAll") }
        }

        Platform.Menu {
            objectName: "viewMenu"
            title: "View"
            // macOS adds its own Enter/Exit Full Screen item (Control-Command-F) to the
            // View menu; one of ours would show a second copy.
            Platform.MenuItem {
                text: root.showPath ? "Hide Path" : "Show Path"
                enabled: root.pathAvailable
                role: Platform.MenuItem.NoRole
                onTriggered: root.pathToggleRequested()
            }
            Platform.MenuSeparator {}
            Platform.Menu {
                objectName: "appearanceMenu"
                title: "Appearance"
                Platform.MenuItemGroup { id: appearanceGroup; exclusive: true }
                Platform.MenuItem { id: lightItem; text: "Light"; checkable: true; group: appearanceGroup; role: Platform.MenuItem.NoRole; onTriggered: { root.appearanceRequested("light"); root.syncAppearance() } }
                Platform.MenuItem { id: darkItem; text: "Dark"; checkable: true; group: appearanceGroup; role: Platform.MenuItem.NoRole; onTriggered: { root.appearanceRequested("dark"); root.syncAppearance() } }
                Platform.MenuItem { id: systemItem; text: "System"; checkable: true; group: appearanceGroup; role: Platform.MenuItem.NoRole; onTriggered: { root.appearanceRequested("system"); root.syncAppearance() } }
            }
        }

        Platform.Menu {
            objectName: "windowMenu"
            title: "Window"
            Platform.MenuItem { text: "Minimize"; shortcut: "Ctrl+M"; role: Platform.MenuItem.NoRole; onTriggered: root.appWindow.showMinimized() }
            Platform.MenuItem {
                text: "Zoom"
                role: Platform.MenuItem.NoRole
                onTriggered: root.appWindow.visibility === Window.Maximized ? root.appWindow.showNormal() : root.appWindow.showMaximized()
            }
            Platform.MenuSeparator {}
            Platform.MenuItem {
                text: "Bring All to Front"
                role: Platform.MenuItem.NoRole
                onTriggered: { root.appWindow.raise(); root.appWindow.requestActivate() }
            }
        }

        Platform.Menu {
            objectName: "helpMenu"
            title: "Help"
            Platform.MenuItem { text: "HamClock Pro Help"; role: Platform.MenuItem.NoRole; onTriggered: Qt.openUrlExternally(root.supportUrl) }
            // Moved into the application menu by their roles. Settings… is app-specific:
            // the Preferences role would retitle it "Preferences...".
            Platform.MenuItem { objectName: "aboutItem"; text: "About HamClock Pro"; role: Platform.MenuItem.AboutRole; onTriggered: about.open() }
            Platform.MenuItem {
                objectName: "settingsItem"
                text: "Settings…"
                shortcut: "Ctrl+," // Command-comma
                enabled: root.settingsAvailable
                role: Platform.MenuItem.ApplicationSpecificRole
                onTriggered: root.settingsRequested()
            }
        }
    }
}
