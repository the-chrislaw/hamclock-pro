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

    // Each key: label t, typed text c (or key code k, or action a), width in key units u.
    // Keys are placed at explicit x positions (no Row/Repeater ordering), so the layout is
    // identical every time the keyboard opens.
    readonly property var rows: [
        "1234567890".split("").map(ch => ({ c: ch })),
        "qwertyuiop".split("").map(ch => ({ c: ch })),
        "asdfghjkl".split("").map(ch => ({ c: ch })).concat([{ t: "\u232b", k: Qt.Key_Backspace, name: "Backspace" }]),
        [{ t: "\u21e7", a: "shift", name: "Shift" }].concat("zxcvbnm/.".split("").map(ch => ({ c: ch }))),
        [{ c: "-" }, { c: ":" }, { c: "@" }, { t: "space", c: " ", u: 3 },
         { t: "\u2190", k: Qt.Key_Left, name: "Left" }, { t: "\u2192", k: Qt.Key_Right, name: "Right" },
         { t: "\u21b5", k: Qt.Key_Return, name: "Enter" }, { t: "Hide", a: "hide", name: "Hide keyboard" }]
    ]
    readonly property real keyGap: Theme.gap * 0.6

    function activate(spec) {
        if (spec.a === "shift") shifted = !shifted
        else if (spec.a === "hide") target.focus = false  // leaves the field; Settings > Done still saves
        else if (spec.k !== undefined) key(spec.k)
        else tap(spec.c)
    }

    contentItem: Item {
        implicitHeight: keyboard.rows.length * keyboard.keyHeight + (keyboard.rows.length - 1) * keyboard.keyGap
        Repeater {
            model: keyboard.rows.length
            Item {
                id: row
                required property int index
                readonly property var keys: keyboard.rows[index]
                y: index * (keyboard.keyHeight + keyboard.keyGap)
                width: parent.width
                height: keyboard.keyHeight
                Repeater {
                    model: row.keys.length
                    Key {
                        required property int index
                        readonly property var spec: row.keys[index]
                        readonly property int unitsBefore: row.keys.slice(0, index).reduce((n, s) => n + (s.u || 1), 0)
                        x: unitsBefore * (keyboard.keyWidth + keyboard.keyGap)
                        units: spec.u || 1
                        label: spec.t !== undefined ? spec.t : (keyboard.shifted ? spec.c.toUpperCase() : spec.c)
                        accent: spec.a === "hide" || (spec.a === "shift" && keyboard.shifted)
                        Accessible.name: spec.name || label
                        onActivated: keyboard.activate(spec)
                    }
                }
            }
        }
    }
}
