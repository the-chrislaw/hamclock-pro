import QtQuick
import QtQuick.Layouts
import HamClock

// Standard widget card: title + live/stale badge, content, status line.
// Widgets put their content inside; `widget` is their WidgetModel (optional).
Rectangle {
    id: card
    property string title: widget ? widget.title : ""
    property QtObject widget: null
    property bool showStatus: widget !== null
    default property alias content: body.data

    radius: Theme.radius
    color: Theme.surface
    border.color: activeFocus ? Theme.focus : Theme.divider
    border.width: activeFocus ? 2 : 1
    activeFocusOnTab: true
    implicitHeight: column.implicitHeight + 2 * Theme.pad
    Accessible.role: Accessible.Grouping
    Accessible.name: title + (widget ? ", " + Theme.stateLabel(widget.state) + ", " + widget.statusText : "")

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap * 0.6

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap * 0.6
            Text {
                text: card.title
                color: Theme.ink
                font.pixelSize: Theme.fontTitle
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            StatusBadge { visible: card.showStatus; status: widget ? widget.state : "" }
        }
        Item {
            id: body
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: childrenRect.height
        }
        Text {
            visible: card.showStatus && text.length > 0
            text: widget ? widget.statusText + (widget.ageText ? " · " + widget.ageText : "") : ""
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
}
