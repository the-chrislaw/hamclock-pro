import QtQuick
import QtQuick.Controls.Basic
import HamClock

// Modal list of widgets for one pane slot; arrow keys + Enter work for a TV remote.
Popup {
    id: chooser
    property int slot: 0
    readonly property real roomW: parent ? parent.width : 640
    readonly property real roomH: parent ? parent.height : 400
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: Theme.pad
    width: Math.min(roomW - 2 * Theme.pad, 360 * Theme.scale)
    height: Math.min(roomH - 2 * Theme.pad, list.contentHeight + 2 * Theme.pad + titleText.height + Theme.gap)
    anchors.centerIn: parent

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surfaceRaised
        border.color: Theme.divider
    }
    onOpened: {
        const current = appSettings.panes[slot]
        list.currentIndex = 0
        for (let i = 0; i < widgetRegistry.count; ++i) {
            const row = list.itemAtIndex(i)
            if (row && row.key === current) list.currentIndex = i
        }
        list.forceActiveFocus()
    }

    contentItem: Column {
        spacing: Theme.gap
        Text {
            id: titleText
            text: "Show in pane " + (chooser.slot + 1)
            color: Theme.ink
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
        }
        ListView {
            id: list
            width: parent.width
            height: Math.min(contentHeight, chooser.roomH - 6 * Theme.pad - titleText.height)
            clip: true
            model: widgetRegistry
            keyNavigationEnabled: true
            boundsBehavior: Flickable.StopAtBounds
            footer: Item { width: list.width; height: Theme.touch
                AppButton {
                    anchors.fill: parent; text: "Empty pane"
                    onClicked: { appSettings.setPane(chooser.slot, "none"); chooser.close() }
                }
            }
            delegate: ItemDelegate {
                width: list.width
                height: Theme.touch
                highlighted: ListView.isCurrentItem
                readonly property string key: widgetId
                text: title
                font.pixelSize: Theme.fontBody
                onClicked: { appSettings.setPane(chooser.slot, widgetId); chooser.close() }
                Keys.onReturnPressed: clicked()
                Keys.onEnterPressed: clicked()
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: appSettings.panes[chooser.slot] === widgetId ? Theme.accent : Theme.ink
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.radius / 2
                    color: parent.down ? Theme.divider : "transparent"
                    border.color: parent.activeFocus || parent.highlighted ? Theme.focus : "transparent"
                    border.width: 2
                }
            }
        }
    }
}
