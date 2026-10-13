import QtQuick
import HamClock

// One dashboard pane: shows the widget chosen for this slot (appSettings.panes),
// or a placeholder when the slot is empty. Tapping anywhere asks to change it; swiping steps.
Item {
    id: slot
    property int index: 0
    readonly property string choice: appSettings.panes[index]
    signal chooseRequested(int slot)

    // Step to the next (+1) or previous (-1) widget in registry order; "none" sits
    // between the last and first widget so a swipe can also empty the pane.
    function step(direction) {
        const ids = ["none"]
        for (let i = 0; i < loaders.count; ++i) ids.push(loaders.itemAt(i).wid)
        const at = Math.max(0, ids.indexOf(choice))
        appSettings.setPane(index, ids[(at + direction + ids.length) % ids.length])
    }

    Repeater {
        id: loaders
        model: widgetRegistry
        delegate: Loader {
            readonly property string wid: widgetId
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

    // Tap anywhere on the pane to change it; swipe sideways to step. Handlers are
    // passive so widgets and the title button keep working.
    TapHandler { onTapped: slot.chooseRequested(slot.index) }
    DragHandler {
        target: null
        xAxis.enabled: true
        yAxis.enabled: false
        onActiveChanged: {
            if (active) return
            const dx = centroid.position.x - centroid.pressPosition.x
            if (Math.abs(dx) > 60 * Theme.scale) slot.step(dx < 0 ? 1 : -1)
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
            text: "Tap to choose a widget."
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
