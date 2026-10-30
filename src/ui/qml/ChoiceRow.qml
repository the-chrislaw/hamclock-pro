import QtQuick
import QtQuick.Layouts
import HamClock

// One labelled setting inside a Settings card: a caption, an optional note, and a row
// of choices that wraps on narrow screens. The selected choice is filled; picking one
// applies at once through `chosen` (Settings has no Save buttons).
ColumnLayout {
    id: row
    property string label: ""
    property string note: ""
    // [[value, text], ...]
    property var choices: []
    property var current
    signal chosen(var value)
    spacing: Theme.gap * 0.4
    Layout.fillWidth: true

    Text { text: row.label; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
           wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Text { visible: row.note.length > 0; text: row.note; color: Theme.secondary; font.pixelSize: Theme.fontCaption
           wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Flow {
        Layout.fillWidth: true
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
