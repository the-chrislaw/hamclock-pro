import QtQuick
import HamClock

// One dashboard pane: shows the widget chosen for this slot (appSettings.panes),
// or a placeholder when the slot is empty. Tapping the title asks to change it.
Item {
    id: slot
    property int index: 0
    readonly property string choice: appSettings.panes[index]
    signal chooseRequested(int slot)

    Repeater {
        model: widgetRegistry
        delegate: Loader {
            anchors.fill: parent
            active: widgetId === slot.choice
            visible: active
            source: card
            onLoaded: {
                item.widget = widgetModel
                item.chooser = true
                item.titleActivated.connect(function () { slot.chooseRequested(slot.index) })
            }
        }
    }

    // Empty slot, or a saved choice whose widget no longer exists.
    Card {
        anchors.fill: parent
        title: "Empty pane"
        chooser: true
        visible: slot.choice === "none" || !widgetRegistry.model(slot.choice)
        onTitleActivated: slot.chooseRequested(slot.index)
        Text {
            width: parent.width
            text: "Tap the title to choose a widget."
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
