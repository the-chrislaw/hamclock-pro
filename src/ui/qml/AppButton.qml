import QtQuick
import QtQuick.Controls.Basic
import HamClock

// Rounded, flat button with a clear pressed state and focus ring.
// `primary` fills it with the accent colour. confirm() briefly swaps the label for
// "✓ Saved" (or the given text) so a save that changes nothing visible still answers.
Button {
    id: control
    property bool primary: false
    property string confirmation: ""
    readonly property int confirmMs: 2000
    function confirm(message) {
        confirmation = message === undefined ? "✓ Saved" : message
        confirmTimer.restart()
    }
    Timer { id: confirmTimer; interval: control.confirmMs; onTriggered: control.confirmation = "" }
    Accessible.name: confirmation.length ? confirmation : text
    implicitHeight: Theme.touch
    leftPadding: Theme.pad
    rightPadding: Theme.pad
    // Size from the content plus the padding the control really applies, so labels never elide.
    implicitWidth: Math.max(Theme.touch, implicitContentWidth + leftPadding + rightPadding)
    font.pixelSize: Theme.fontBody
    font.weight: Font.DemiBold
    focusPolicy: Qt.StrongFocus
    contentItem: Text {
        text: control.confirmation.length ? control.confirmation : control.text
        font: control.font
        color: control.primary ? Theme.accentInk : Theme.ink
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: height / 2
        color: control.primary ? (control.down ? Qt.darker(Theme.accent, 1.15) : Theme.accent)
                               : (control.down ? Theme.divider : (control.hovered ? Theme.surfaceRaised : "transparent"))
        border.color: control.activeFocus ? Theme.focus : (control.primary ? "transparent" : Theme.divider)
        border.width: control.activeFocus ? 2 : 1
        Behavior on color { ColorAnimation { duration: 90 } }
    }
}
