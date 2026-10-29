import QtQuick
import HamClock

Card {
    id: root
    // The picture fills the body; the channel name is overlaid so it also fits compact slots.
    Item {
        width: parent.width
        height: Math.max(80 * Theme.scale, (root.height / root.fit) - 2 * Theme.pad - 3 * Theme.gap - 2 * Theme.fontCaption - Theme.fontTitle)
        Image {
            id: sun
            anchors.fill: parent
            visible: root.widget && root.widget.hasImage
            source: root.widget && root.widget.hasImage ? root.widget.imageUrl : ""
            fillMode: Image.PreserveAspectFit
            smooth: true
            // Local data: URL decode is cheap; synchronous so a render or screenshot never catches it blank.
            asynchronous: false
            sourceSize.width: 512
            sourceSize.height: 512
            Accessible.role: Accessible.Graphic
            Accessible.name: root.widget ? "Sun in " + root.widget.channelName : "Sun"
        }
        Text {
            anchors.centerIn: parent
            visible: !sun.visible
            text: "No image yet"
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
        }
        Text {
            anchors { left: parent.left; bottom: parent.bottom; margins: Theme.gap * 0.5 }
            text: root.widget ? root.widget.channelName + (root.compact ? "" : " · " + root.widget.channelCaption) : ""
            color: "#ffffff"
            style: Text.Outline
            styleColor: "#000000"
            font.pixelSize: Theme.fontCaption
            width: parent.width - Theme.gap
            elide: Text.ElideRight
        }
        TapHandler {
            enabled: root.widget && root.widget.channelCount > 1
            onTapped: root.widget.cycle()
        }
    }
}
