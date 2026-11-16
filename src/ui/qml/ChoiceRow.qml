import QtQuick
import QtQuick.Layouts
import HamClock

// One labelled setting inside a Settings card: a caption, an optional note, and a row
// of choices that wraps on narrow screens. The selected choice is filled; picking one
// applies at once through `chosen` (Settings has no Save buttons).
// A Column, not a ColumnLayout: its wrapped caption and Flow settle in its own polish
// instead of inside the card's layout pass, which on Qt 6.5 counted each rewrap as a
// polish loop when the width changed (iPhone rotation) and stopped laying the card out.
Column {
    id: row
    property string label: ""
    property string note: ""
    // [[value, text], ...]
    property var choices: []
    property var current
    signal chosen(var value)
    spacing: Theme.gap * 0.4
    Layout.fillWidth: true
    // Share the card's width instead of asking for the caption's full length.
    Layout.preferredWidth: 0

    Text { text: row.label; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
           wrapMode: Text.WordWrap; width: row.width }
    Text { visible: row.note.length > 0; text: row.note; color: Theme.secondary; font.pixelSize: Theme.fontCaption
           wrapMode: Text.WordWrap; width: row.width }
    Flow {
        width: row.width
        spacing: Theme.gap * 0.6
        Repeater {
            model: row.choices
            AppButton {
                text: modelData[1]
                primary: row.current === modelData[0]
                Accessible.name: row.label + ": " + modelData[1]
                onClicked: row.chosen(modelData[0])
            }
        }
    }
}
