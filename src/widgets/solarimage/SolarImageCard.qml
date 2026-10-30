import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Channel name above the picture (never over it); the picture takes the rest of the body.
    ColumnLayout {
        width: parent.width
        height: parent.height
        spacing: Theme.gap * 0.4
        Text {
            text: root.widget ? root.widget.channelName + (root.compact ? "" : " · " + root.widget.channelCaption) : ""
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight
            objectName: "elideOk"  // the channel caption is a description; the name comes first
            Layout.fillWidth: true
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 60 * Theme.scale
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
            TapHandler {
                enabled: root.widget && root.widget.channelCount > 1
                onTapped: root.widget.cycle()
            }
        }
    }
}
