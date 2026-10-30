import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HamClock

// A collapsed Settings group: one row with the title, a one-line summary of what is set
// and a chevron. Tap (or Enter/Space) to open it in place; its contents are the children.
// `boxed` draws it as a card of its own (top level); inside a card it is a plain row.
Rectangle {
    id: group
    property string title: ""
    property string summary: ""
    property bool expanded: false
    property bool boxed: true
    default property alias content: body.data

    implicitHeight: column.implicitHeight + (boxed ? 2 * Theme.pad : 0)
    radius: boxed ? Theme.radius : 0
    color: boxed ? Theme.surface : "transparent"
    border.color: boxed ? Theme.divider : "transparent"
    border.width: boxed ? 1 : 0
    Accessible.role: Accessible.Grouping
    Accessible.name: title

    ColumnLayout {
        id: column
        x: group.boxed ? Theme.pad : 0
        y: group.boxed ? Theme.pad : 0
        width: group.width - 2 * x
        spacing: Theme.gap * 0.8

        Button {
            id: header
            Layout.fillWidth: true
            implicitHeight: Theme.touch
            padding: 0
            focusPolicy: Qt.StrongFocus
            Accessible.name: group.title + (group.summary.length ? ", " + group.summary : "")
                             + (group.expanded ? ", open" : ", closed")
            onClicked: group.expanded = !group.expanded
            background: Rectangle {
                radius: Theme.radius * 0.6
                color: header.down ? Theme.divider : (header.hovered ? Theme.surfaceRaised : "transparent")
                border.color: header.activeFocus ? Theme.focus : "transparent"
                border.width: header.activeFocus ? 2 : 0
            }
            contentItem: RowLayout {
                spacing: Theme.gap
                Text { text: group.title; color: Theme.ink; font.pixelSize: group.boxed ? Theme.fontTitle : Theme.fontBody
                       font.weight: Font.DemiBold }
                // The summary is a hint; when the row is narrow it is cut with an ellipsis on purpose.
                Text { objectName: "elideOk"; text: group.summary; color: Theme.secondary; font.pixelSize: Theme.fontCaption
                       elide: Text.ElideRight; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }
                Text { text: "›"; color: Theme.secondary; font.pixelSize: Theme.fontTitle; rotation: group.expanded ? 90 : 0
                       Behavior on rotation { NumberAnimation { duration: 120 } } }
            }
        }
        ColumnLayout {
            id: body
            visible: group.expanded
            Layout.fillWidth: true
            spacing: Theme.gap * 0.8
        }
    }
}
