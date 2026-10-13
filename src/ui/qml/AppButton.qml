import QtQuick
import QtQuick.Controls.Basic
import HamClock

// Rounded, flat button with a clear pressed state and focus ring.
// `primary` fills it with the accent colour.
Button {
    id: control
    property bool primary: false
    implicitHeight: Theme.touch
    implicitWidth: Math.max(Theme.touch, contentItem.implicitWidth + 2 * Theme.pad)
    font.pixelSize: Theme.fontBody
    font.weight: Font.DemiBold
    focusPolicy: Qt.StrongFocus
    contentItem: Text {
        text: control.text
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
