import QtQuick
import QtQuick.Layouts
import HamClock

// One labelled value: small label, big number, optional caption.
// Shows an em dash when `available` is false — never a made-up number.
ColumnLayout {
    property string label: ""
    property string value: ""
    property string caption: ""
    property bool available: true
    // Long labels (a weather place name) take a second line instead of being cut short.
    property bool wrapLabel: false
    property color valueColor: Theme.ink
    spacing: 2 * Theme.scale
    Accessible.role: Accessible.StaticText
    Accessible.name: label + ": " + (available ? value + (caption ? ", " + caption : "") : "not available")

    Text {
        text: parent.label
        color: Theme.secondary
        font.pixelSize: Theme.fontCaption
        font.weight: Font.DemiBold
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 0.6
        wrapMode: parent.wrapLabel ? Text.WordWrap : Text.NoWrap
        maximumLineCount: parent.wrapLabel ? 2 : 1
        elide: Text.ElideRight
        Layout.fillWidth: true
    }
    Text {
        text: parent.available ? parent.value : "—"
        color: parent.available ? parent.valueColor : Theme.secondary
        font.pixelSize: Theme.fontValue
        font.weight: Font.DemiBold
        // A value wider than the slot shrinks to fit instead of running out of the card.
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: Theme.fontBody
        Layout.fillWidth: true
    }
    Text {
        visible: text.length > 0
        text: parent.available ? parent.caption : ""
        color: Theme.secondary
        font.pixelSize: Theme.fontCaption
        elide: Text.ElideRight
        Layout.fillWidth: true
    }
}
