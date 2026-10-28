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
    readonly property bool compact: width < 420 * Theme.scale || height < 190 * Theme.scale
    // When true (set by the pane slot) tapping the title or pressing Enter asks to change the widget.
    property bool chooser: false
    // Content sized by the card (map): its size must not feed back into the card's own.
    property bool fillBody: false
    // False hides the title row (dense screens, map card): the body gets every pixel.
    property bool showTitle: true
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
            visible: card.showTitle
            Layout.fillWidth: true
            spacing: Theme.gap * 0.6
            Text {
                id: titleText
                text: card.title + (card.chooser ? " ▾" : "")
                color: Theme.ink
                font.pixelSize: Theme.fontTitle
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                maximumLineCount: 2
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
            // Title first: when the title plus the full badge does not fit on one line, the badge drops to a dot.
            TextMetrics { id: titleMetrics; text: titleText.text; font: titleText.font }
            StatusBadge {
                id: badge
                visible: card.showStatus
                status: widget ? widget.state : ""
                dotOnly: titleMetrics.advanceWidth + fullWidth + Theme.gap * 0.6 > column.width
            }
        }
        Item {
            id: body
            clip: true
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: card.fillBody ? 0 : childrenRect.height
        }
        RowLayout {
            visible: card.showStatus && (statusLine.text.length > 0 || ageLine.text.length > 0)
            Layout.fillWidth: true
            spacing: Theme.gap * 0.6
            // The status reason may be shortened; the data age never is.
            Text {
                id: statusLine
                objectName: "elideOk"  // long error reasons are shortened on purpose
                text: widget ? widget.statusText : ""
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
                elide: Text.ElideRight
                wrapMode: Text.WordWrap
                maximumLineCount: card.compact ? 1 : 2
                Layout.fillWidth: true
            }
            Text {
                id: ageLine
                text: widget ? widget.ageText : ""
                visible: text.length > 0
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
                Layout.alignment: Qt.AlignTop
                Layout.minimumWidth: implicitWidth
            }
        }
    }
}
