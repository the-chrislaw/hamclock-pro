import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HamClock

// Labelled text field sized for touch.
ColumnLayout {
    property alias label: caption.text
    property alias text: field.text
    property alias placeholder: field.placeholderText
    property alias echoMode: field.echoMode
    property alias inputMethodHints: field.inputMethodHints
    property alias field: field
    spacing: 4 * Theme.scale
    Layout.fillWidth: true

    Text { id: caption; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
    TextField {
        id: field
        Layout.fillWidth: true
        implicitHeight: Theme.touch
        font.pixelSize: Theme.fontBody
        color: Theme.ink
        placeholderTextColor: Theme.secondary
        selectByMouse: true
        Accessible.name: caption.text
        background: Rectangle {
            radius: 10 * Theme.scale
            color: Theme.surfaceRaised
            border.color: field.activeFocus ? Theme.focus : Theme.divider
            border.width: field.activeFocus ? 2 : 1
        }
    }
}
