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
    height: Math.min(roomH - 2 * Theme.pad, list.contentHeight + 2 * Theme.pad + titleText.height + tabs.height + 2 * Theme.gap + Theme.touch)
    anchors.centerIn: parent

    // Category tabs narrow the list (A8). Unknown widget ids fall under "Other".
    readonly property var categoryNames: ["All", "Space Wx", "DX & Contest", "Weather", "Sky & Space", "Station", "Other"]
    readonly property var categoryOf: ({
        spaceweather: 1, solaralerts: 1, solarscales: 1, solaraurora: 1, solartrend: 1, solarwind: 1, solarimage: 1, bands: 1,
        dxcluster: 2, pskreporter: 2, beacons: 2, ota: 2, watchlist: 2, lookup: 2, adif: 2, hamalert: 2,
        weather: 3, firewx: 3, fires: 3, storms: 3, quakes: 3, marine: 3,
        sunmoon: 4, satellites: 4,
        sensors: 5, station: 5
    })
    property int category: 0
    function inCategory(id) {
        return category === 0 || (categoryOf[id] !== undefined ? categoryOf[id] : 6) === category
    }

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surfaceRaised
        border.color: Theme.divider
    }
    onOpened: {
        const current = appSettings.panes[slot]
        category = current in categoryOf ? categoryOf[current] : 0
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
        Flow {
            id: tabs
            width: parent.width
            spacing: Theme.gap / 2
            Repeater {
                model: chooser.categoryNames
                delegate: AppButton {
                    required property int index
                    required property string modelData
                    text: modelData
                    primary: chooser.category === index
                    onClicked: chooser.category = index
                }
            }
        }
        ListView {
            id: list
            width: parent.width
            height: Math.min(contentHeight, chooser.roomH - 6 * Theme.pad - titleText.height - tabs.height)
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
                visible: chooser.inCategory(widgetId)
                height: visible ? Theme.touch : 0
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
