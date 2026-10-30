import QtQuick
import QtQuick.Layouts
import HamClock

// In-app satellite pass alert: "SO-50 in 4 min · AOS 14:05 · max 54° · NW → SE · FM ...".
// The only alert on the Pi, Linux and Windows. On iPhone, iPad and Mac the system
// notification announces the pass, so this shows only while the satellite is up.
Rectangle {
    id: banner
    visible: satAlerts.bannerText.length > 0 && (!satAlerts.systemNotifications || satAlerts.bannerLive)
    height: row.implicitHeight + Theme.pad
    radius: Theme.radius
    color: satAlerts.bannerLive ? Theme.good : Theme.accent
    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Theme.pad / 2
        spacing: Theme.gap
        Text {
            objectName: "elideOk"
            Layout.fillWidth: true
            text: satAlerts.bannerText
            color: Theme.canvas
            font.pixelSize: Theme.fontBody
            font.bold: true
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
        AppButton { text: "Dismiss"; onClicked: satAlerts.dismissBanner() }
    }
}
