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

    readonly property real inset: boxed ? Theme.pad : 0
    readonly property real contentWidth: width - 2 * inset

    implicitHeight: column.implicitHeight + 2 * inset
    radius: boxed ? Theme.radius : 0
    color: boxed ? Theme.surface : "transparent"
    border.color: boxed ? Theme.divider : "transparent"
    border.width: boxed ? 1 : 0
    Accessible.role: Accessible.Grouping
    Accessible.name: title

    // A plain Column, not a layout: a layout leaves a hidden child at zero width, and the
    // closed body's wrapped labels then loop the layout polish (seen on iPhone, #697).
    Column {
        id: column
        x: group.inset
        y: group.inset
        width: group.contentWidth > 0 ? group.contentWidth : implicitWidth
        spacing: Theme.gap * 0.8

        Button {
            id: header
            width: column.width
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
            width: column.width
            spacing: Theme.gap * 0.8
        }
    }
}
