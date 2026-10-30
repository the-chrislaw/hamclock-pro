import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import HamClock

// On-screen keyboard for touch-only kiosks (--touch-keyboard). Shows while a text field
// has focus and types into it through keyInjector (real key events). A popup so it stays
// above other popups, such as the time zone search; it never takes focus itself.
Popup {
    id: keyboard
    // The focused item; the keyboard shows only for editable text.
    property Item target: null
    readonly property bool wanted: target !== null && target.cursorPosition !== undefined && !target.readOnly
    property bool shifted: false

    parent: Overlay.overlay
    z: 1000
    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose
    padding: Theme.gap
    width: parent ? parent.width : 0
    x: 0
    y: parent ? parent.height - height : 0
    visible: wanted
    // Upper case by default for upper-case fields (callsign), otherwise lower case.
    onTargetChanged: shifted = target !== null && target.inputMethodHints !== undefined
                     && (target.inputMethodHints & Qt.ImhUppercaseOnly) !== 0

    readonly property real keyHeight: Math.max(36, Math.min(Theme.touch, (parent ? parent.height : 600) * 0.085))
    readonly property real keyWidth: (width - 2 * padding - 9 * Theme.gap * 0.6) / 10

    background: Rectangle { color: Theme.surface; border.color: Theme.divider }

    function tap(text) {
        blanking.touch()
        keyInjector.type(target, shifted ? text.toUpperCase() : text.toLowerCase())
    }
    function key(code) { blanking.touch(); keyInjector.press(target, code) }

    component Key: Rectangle {
        id: k
        property string label
        property real units: 1
        property bool accent: false
        signal activated()
        width: keyboard.keyWidth * units + Theme.gap * 0.6 * (units - 1)
        height: keyboard.keyHeight
        radius: 6 * Theme.scale
        color: area.pressed ? Theme.divider : (accent ? Theme.accent : Theme.surfaceRaised)
        border.color: Theme.divider
        Text {
            anchors.centerIn: parent
            text: k.label
            color: k.accent ? Theme.accentInk : Theme.ink
            font.pixelSize: Theme.fontBody
            font.weight: Font.DemiBold
        }
        MouseArea { id: area; anchors.fill: parent; onClicked: k.activated() }
    }

    contentItem: Column {
        spacing: Theme.gap * 0.6
        Repeater {
            model: ["1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm/."]
            Row {
                required property string modelData
                required property int index
                spacing: Theme.gap * 0.6
                anchors.horizontalCenter: parent.horizontalCenter
                Key {
                    visible: index === 3
                    label: "⇧"
                    accent: keyboard.shifted
                    onActivated: keyboard.shifted = !keyboard.shifted
                    Accessible.name: "Shift"
                }
                Repeater {
                    model: modelData.split("")
                    Key {
                        required property string modelData
                        label: keyboard.shifted ? modelData.toUpperCase() : modelData
                        onActivated: keyboard.tap(modelData)
                    }
                }
                Key {
                    visible: index === 2
                    label: "⌫"
                    onActivated: keyboard.key(Qt.Key_Backspace)
                    Accessible.name: "Backspace"
                }
            }
        }
        Row {
            spacing: Theme.gap * 0.6
            anchors.horizontalCenter: parent.horizontalCenter
            Repeater {
                model: ["-", ":", "@"]
                Key { required property string modelData; label: modelData; onActivated: keyboard.tap(modelData) }
            }
            Key { units: 3; label: "space"; onActivated: keyboard.tap(" ") }
            Key { label: "←"; onActivated: keyboard.key(Qt.Key_Left); Accessible.name: "Left" }
            Key { label: "→"; onActivated: keyboard.key(Qt.Key_Right); Accessible.name: "Right" }
            Key { label: "↵"; onActivated: keyboard.key(Qt.Key_Return); Accessible.name: "Enter" }
            // Hides the keyboard by leaving the field; Settings > Done still saves.
            Key { label: "Hide"; accent: true; onActivated: keyboard.target.focus = false; Accessible.name: "Hide keyboard" }
        }
    }
}
