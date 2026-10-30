import QtQuick
import HamClock
Card {
    id: root
    readonly property var colors: ({ w1: "#487991", w2: "#2f7d3f", w3: "#8a5f00", w4: "#b3362b", w5: "#6c4fa0", w6: "#637074" })
    Component.onCompleted: Stubs.registry.created++
    Rectangle {
        width: parent.width; height: 120
        radius: 8
        color: root.widget ? root.colors[root.widget.widgetId] : "grey"
        Text { anchors.centerIn: parent; color: "white"; font.pixelSize: 30; font.bold: true; text: root.widget ? root.widget.widgetId.toUpperCase() : "" }
    }
}
