import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HamClock

// Time zone chooser: a button showing the current zone opens a searchable list
// (city or region, with the current UTC offset). Touch, keyboard and remote:
// type to filter, Up/Down to move, Enter to pick, Escape to cancel.
ColumnLayout {
    id: picker
    property string zoneId: ""
    property alias label: caption.text
    spacing: 4 * Theme.scale
    Layout.fillWidth: true

    Text { id: caption; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
    AppButton {
        Layout.fillWidth: true
        text: picker.zoneId.length
              ? (timeZones.labelFor(picker.zoneId) || picker.zoneId) + "  •  " + picker.zoneId
              : "Choose a time zone"
        onClicked: popup.open()
        Accessible.name: caption.text
    }

    Popup {
        id: popup
        parent: Overlay.overlay
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.pad
        width: Math.min(parent.width - 2 * Theme.pad, 420 * Theme.scale)
        // Stays above the on-screen keyboard so the search field and results remain visible.
        height: Math.min(parent.height - 2 * Theme.pad - Theme.keyboardInset, 520 * Theme.scale)
        x: (parent.width - width) / 2
        y: Math.max(Theme.pad, (parent.height - Theme.keyboardInset - height) / 4)
        background: Rectangle { radius: Theme.radius; color: Theme.surfaceRaised; border.color: Theme.divider }
        onOpened: { search.text = ""; results.model = timeZones.search("", 80); search.forceActiveFocus() }

        contentItem: ColumnLayout {
            spacing: Theme.gap
            TextField {
                id: search
                Layout.fillWidth: true
                implicitHeight: Theme.touch
                placeholderText: "Search city or region"
                placeholderTextColor: Theme.secondary
                color: Theme.ink
                font.pixelSize: Theme.fontBody
                background: Rectangle { radius: 10 * Theme.scale; color: Theme.canvas
                                        border.color: Theme.focus; border.width: 2 }
                onTextChanged: { results.model = timeZones.search(text, 80); results.currentIndex = 0 }
                Keys.onDownPressed: results.incrementCurrentIndex()
                Keys.onUpPressed: results.decrementCurrentIndex()
                onAccepted: if (results.currentItem) results.currentItem.pick()
            }
            AppButton {
                visible: timeZones.systemZone().length > 0
                text: "Use this device's zone (" + timeZones.systemZone() + ")"
                onClicked: { picker.zoneId = timeZones.systemZone(); popup.close() }
            }
            ListView {
                id: results
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                highlightMoveDuration: 0
                ScrollIndicator.vertical: ScrollIndicator {}
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    width: results.width
                    height: Theme.touch
                    highlighted: ListView.isCurrentItem
                    function pick() { picker.zoneId = modelData.id; popup.close() }
                    onClicked: pick()
                    contentItem: RowLayout {
                        Text { text: modelData.label; color: Theme.ink; font.pixelSize: Theme.fontBody
                               elide: Text.ElideRight; Layout.fillWidth: true }
                        Text { text: modelData.id; color: Theme.secondary; font.pixelSize: Theme.fontCaption
                               elide: Text.ElideLeft; Layout.maximumWidth: results.width * 0.5 }
                    }
                    background: Rectangle { color: highlighted ? Theme.surface : "transparent"; radius: 8 * Theme.scale }
                }
                Text {
                    anchors.centerIn: parent
                    visible: results.count === 0
                    text: "No matching zone"
                    color: Theme.secondary
                    font.pixelSize: Theme.fontBody
                }
            }
        }
    }
}
