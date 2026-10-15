import QtQuick
import HamClock

// One dashboard pane: shows the widget chosen for this slot (appSettings.panes),
// or a placeholder when the slot is empty. Tapping anywhere asks to change it; swiping steps.
Item {
    id: slot
    property int index: 0
    readonly property string saved: appSettings.panes[index]
    readonly property int rotateSeconds: appSettings.paneRotations[index]
    // What is on screen: the saved choice, or the current stop of an automatic
    // rotation (never written to settings, so a rotation does not wear the SD card).
    property string rotated: ""
    readonly property string choice: rotateSeconds > 0 && rotated !== "" ? rotated : saved
    // Widgets the rotation cycles through (empty = every widget).
    readonly property var rotationSet: appSettings.paneRotationSets[index]
    // Pause rotation while the user is interacting or a menu is open.
    property bool paused: false
    onSavedChanged: rotated = ""
    onRotateSecondsChanged: rotated = ""
    onRotationSetChanged: rotated = ""
    signal chooseRequested(int slot)
    signal optionsRequested(int slot)

    // Step to the next (+1) or previous (-1) widget in registry order; "none" sits
    // between the last and first widget so a swipe can also empty the pane.
    function ids() {
        const list = ["none"]
        for (let i = 0; i < loaders.count; ++i) list.push(loaders.itemAt(i).wid)
        return list
    }
    function step(direction) {
        const list = ids()
        const at = Math.max(0, list.indexOf(choice))
        appSettings.setPane(index, list[(at + direction + list.length) % list.length])
    }

    // Automatic rotation: cycle through the slot's rotation set, or every widget when it is empty.
    Timer {
        interval: slot.rotateSeconds * 1000
        repeat: true
        running: slot.rotateSeconds > 0 && !slot.paused && slot.visible
        onTriggered: {
            const known = slot.ids()
            const list = known.filter(function (id) {
                return id !== "none" && (slot.rotationSet.length === 0 || slot.rotationSet.indexOf(id) >= 0)
            })
            if (list.length === 0) return
            slot.rotated = list[(Math.max(0, list.indexOf(slot.choice)) + 1) % list.length]
        }
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

    // Keyboard / TV remote: the focused card bubbles unhandled keys up to the slot.
    // Left/Right step the pane, Menu (or Shift+F10 via Key_Menu) opens its options.
    // Any key counts as activity for screen blanking (does not consume it).
    Keys.onPressed: function (event) { blanking.touch(); event.accepted = false }
    Keys.onLeftPressed: function (event) { slot.step(-1); event.accepted = true }
    Keys.onRightPressed: function (event) { slot.step(1); event.accepted = true }
    // Up/Down move the focus ring to the previous/next focusable item (same order as Tab).
    Keys.onUpPressed: function (event) { slot.moveFocus(false); event.accepted = true }
    Keys.onDownPressed: function (event) { slot.moveFocus(true); event.accepted = true }
    function moveFocus(forward) {
        var next = slot.nextItemInFocusChain(forward)
        if (next && next !== slot) next.forceActiveFocus(forward ? Qt.TabFocusReason : Qt.BacktabFocusReason)
    }
    Keys.onMenuPressed: function (event) { slot.optionsRequested(slot.index); event.accepted = true }

    // Tap anywhere on the pane to change it; swipe sideways to step. Handlers are
    // passive so widgets and the title button keep working.
    TapHandler { onTapped: slot.chooseRequested(slot.index) }
    // Long-press opens the pane options (a long press does not also emit tapped).
    TapHandler { longPressThreshold: 0.6; onLongPressed: slot.optionsRequested(slot.index) }
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
