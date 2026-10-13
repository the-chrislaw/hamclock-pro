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
    // True in a small dashboard slot: show key numbers only, drop details.
    readonly property bool compact: width < 300 * Theme.scale || height < 190 * Theme.scale
    // When true (set by the pane slot) tapping the title or pressing Enter asks to change the widget.
    property bool chooser: false
    // Content sized by the card (map): its size must not feed back into the card's own.
    property bool fillBody: false
    signal titleActivated()

    radius: Theme.radius
    color: Theme.surface
    border.color: activeFocus ? Theme.focus : Theme.divider
    border.width: activeFocus ? 2 : 1
    activeFocusOnTab: true
    clip: true
    Keys.onReturnPressed: if (chooser) titleActivated()
    Keys.onEnterPressed: if (chooser) titleActivated()
    Keys.onSpacePressed: if (chooser) titleActivated()
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
                id: titleText
                text: card.title + (card.chooser ? " ▾" : "")
                color: Theme.ink
                font.pixelSize: Theme.fontTitle
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.minimumHeight: card.chooser && !Theme.dense ? Theme.touch * 0.7 : 0
                verticalAlignment: Text.AlignVCenter
                Accessible.role: card.chooser ? Accessible.Button : Accessible.StaticText
                Accessible.name: card.title + (card.chooser ? ", change widget" : "")
                MouseArea {
                    anchors.fill: parent
                    enabled: card.chooser
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { card.forceActiveFocus(); card.titleActivated() }
                }
            }
            StatusBadge { visible: card.showStatus; status: widget ? widget.state : "" }
        }
        Item {
            id: body
            clip: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: card.fillBody ? 0 : childrenRect.height
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
