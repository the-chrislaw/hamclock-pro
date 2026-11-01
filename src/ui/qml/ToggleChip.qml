import QtQuick
import HamClock

// On/off chip for Settings: the label never changes, the chip shows the state
// (filled with a check mark when on). The caller saves in onFlipped.
AppButton {
    id: chip
    property bool on: false
    property string label: ""
    signal flipped(bool on)   // the state the user asked for
    text: (on ? "✓ " : "") + label
    primary: on
    checkable: false
    Accessible.role: Accessible.CheckBox
    Accessible.checkable: true
    Accessible.checked: on
    Accessible.name: label
    onClicked: chip.flipped(!on)
}
