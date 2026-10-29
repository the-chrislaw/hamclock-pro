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
    default property alias content: stage.data
    // True in a small dashboard slot: show key numbers only, drop details.
    readonly property bool compact: width < 420 * Theme.scale || height < 190 * Theme.scale

    // Adaptive fit (dashboard panes; the slot turns it on). The content is laid out
    // in a stage 1/fit the size of the body and drawn `fit` times larger, so a pane
    // bigger than its layout needs grows crisply to fill it instead of leaving space.
    // fit stops where the widest layout is reached (designWidth: 420 for the full
    // layout, 260 for the compact one) and where the content's natural height fills
    // the body; content that stretches (lists, plots) fills the stage at any fit.
    property bool adaptive: false
    // A narrow pane with height to spare (desktop pane rows, the station column's
    // spill pane): widgets stack their full layout instead of the compact one.
    readonly property bool tall: compact && width >= 240 * Theme.scale && height >= width * 1.1
    // A compact pane with real height (the two-row grids at 1280x800 and 2560x1440):
    // room for a list or a stacked layout even when it is not taller than wide.
    readonly property bool roomy: compact && height >= 260 * Theme.scale
    // A widget that needs setting up shows why, in full, with a button to Settings.
    readonly property bool needsSetup: adaptive && widget !== null && widget.state === "unconfigured"
    signal setupRequested()
    property real designWidth: (compact ? 260 : 420) * Theme.scale
    property real maxFit: 2.2
    property real fit: 1
    // Natural content height in stage units: what the children ask for, not what they were given.
    readonly property real naturalHeight: {
        let h = 0
        for (let i = 0; i < stage.children.length; ++i) {
            const c = stage.children[i]
            if (c.visible) h = Math.max(h, c.y + Math.max(c.implicitHeight, 1))
        }
        return h
    }
    function refit() {
        if (!adaptive || fillBody || body.width <= 0 || body.height <= 0 || naturalHeight <= 0) { fit = 1; return }
        const target = Math.max(1, Math.min(maxFit, body.width / designWidth, body.height / naturalHeight))
        // Hysteresis: small changes (text ticking, wrapping at the new width) do not re-layout.
        if (Math.abs(target - fit) > 0.03 * fit) fit = Math.round(target * 100) / 100
    }
    onNaturalHeightChanged: Qt.callLater(refit)
    onWidthChanged: Qt.callLater(refit)
    onHeightChanged: Qt.callLater(refit)
    onAdaptiveChanged: Qt.callLater(refit)
    onCompactChanged: Qt.callLater(refit)
    onDesignWidthChanged: Qt.callLater(refit)
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
            implicitHeight: card.fillBody ? 0 : stage.childrenRect.height
            Item {
                id: stage
                width: body.width / card.fit
                height: body.height / card.fit
                scale: card.fit
                transformOrigin: Item.TopLeft
                visible: !card.needsSetup
            }
            ColumnLayout {
                visible: card.needsSetup
                anchors.centerIn: parent
                width: Math.min(parent.width, 360 * Theme.scale)
                spacing: Theme.gap
                Text {
                    text: card.widget ? card.widget.statusText : ""
                    color: Theme.secondary
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                AppButton {
                    text: "Open Settings"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: card.setupRequested()
                }
            }
        }
        RowLayout {
            visible: card.showStatus && !card.needsSetup && (statusLine.text.length > 0 || ageLine.text.length > 0)
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
