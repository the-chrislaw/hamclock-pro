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
    // True once the user types here; Settings > Done saves only the sections they changed.
    property bool edited: false
    spacing: 4 * Theme.scale
    Layout.fillWidth: true
    // Share the row's width instead of asking for the caption's full length, so a
    // long label wraps rather than pushing the field off a phone screen.
    Layout.preferredWidth: 0

    Text { id: caption; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
           wrapMode: Text.WordWrap; Layout.fillWidth: true }
    TextField {
        id: field
        Layout.fillWidth: true
        implicitHeight: Theme.touch
        font.pixelSize: Theme.fontBody
        color: Theme.ink
        placeholderTextColor: Theme.secondary
        selectByMouse: true
        // Typing drops the caller's `text: appSettings.x` binding, so another section
        // saving (which re-evaluates every binding) can't wipe an unsaved edit.
        onTextEdited: { parent.edited = true; text = text }
        Accessible.name: caption.text
        background: Rectangle {
            radius: 10 * Theme.scale
            color: Theme.surfaceRaised
            border.color: field.activeFocus ? Theme.focus : Theme.divider
            border.width: field.activeFocus ? 2 : 1
        }
    }
    // Non-empty only in the iPad app running on a Mac, where keys never reach Qt (#513).
    Text {
        visible: textEntryHint.message !== ""
        text: textEntryHint.message
        color: Theme.warn
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        font.pixelSize: Theme.fontCaption
    }
}
