import QtQuick
import QtQuick.Controls.Basic
import HamClock

// Long-press menu for one pane slot: change, step, empty, or restore the default layout.
Popup {
    id: options
    property int slot: 0
    signal changeRequested(int slot)
    signal stepRequested(int slot, int direction)
    readonly property real roomW: parent ? parent.width : 640
    readonly property real roomH: parent ? parent.height : 400
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: Theme.pad
    width: Math.min(roomW - 2 * Theme.pad, 280 * Theme.scale)
    anchors.centerIn: parent

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surfaceRaised
        border.color: Theme.divider
    }
    onOpened: firstButton.forceActiveFocus()

    contentItem: Column {
        spacing: Theme.gap
        Text {
            text: "Pane " + (options.slot + 1)
            color: Theme.ink
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }
        AppButton {
            id: firstButton
            width: parent.width; text: "Change widget..."
            onClicked: { options.close(); options.changeRequested(options.slot) }
        }
        AppButton {
            width: parent.width; text: "Next widget"
            onClicked: { options.close(); options.stepRequested(options.slot, 1) }
        }
        AppButton {
            width: parent.width; text: "Previous widget"
            onClicked: { options.close(); options.stepRequested(options.slot, -1) }
        }
        AppButton {
            width: parent.width; text: "Stop demo"
            visible: appSettings.demoMode
            onClicked: { appSettings.demoMode = false; options.close() }
        }
        AppButton {
            readonly property var steps: [0, 5, 10, 30, 60]
            readonly property int current: appSettings.paneRotations[options.slot]
            width: parent.width
            text: "Rotate: " + (current > 0 ? "every " + current + " s" : "off")
            onClicked: appSettings.setPaneRotation(options.slot, steps[(steps.indexOf(current) + 1) % steps.length])
        }
        AppButton {
            readonly property var members: appSettings.paneRotationSets[options.slot]
            readonly property string shown: appSettings.panes[options.slot]
            readonly property bool inSet: members.indexOf(shown) >= 0
            width: parent.width
            visible: appSettings.paneRotations[options.slot] > 0 && shown !== "none"
            text: members.length === 0 ? "Rotation: all widgets (tap to pick)"
                  : (inSet ? "Remove from rotation" : "Add to rotation") + " (" + members.length + ")"
            onClicked: {
                // From "all widgets", the first tap starts a set holding just this widget.
                appSettings.togglePaneRotationMember(options.slot, shown)
            }
        }
        AppButton {
            width: parent.width; text: "Rotate all widgets"
            visible: appSettings.paneRotationSets[options.slot].length > 0
            onClicked: appSettings.clearPaneRotationSet(options.slot)
        }
        AppButton {
            width: parent.width; text: "Empty pane"
            onClicked: { appSettings.setPane(options.slot, "none"); options.close() }
        }
        AppButton {
            width: parent.width; text: "Reset all panes"
            onClicked: { appSettings.resetPanes(); options.close() }
        }
    }
}
