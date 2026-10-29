import QtQuick
import QtQuick.Layouts
import HamClock

// Big Clock: UTC and local time in the largest type that fits, opened from the
// Timers page. Tap anywhere (or press Escape) to close.
Item {
    id: root
    signal closeRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Item { Layout.fillHeight: true }
        Canvas {
            id: dial
            visible: appSettings.bigClockFace === 1
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: visible ? Math.min(root.height * 0.42, root.width * 0.5) : 0
            Layout.preferredWidth: Layout.preferredHeight
            // parseInt, not Number: in 12-hour mode the seconds read "18 PM".
            property var parts: station.utcTime.split(":")
            property real hh: parts.length > 2 ? parseInt(parts[0], 10) : 0
            property real mm: parts.length > 2 ? parseInt(parts[1], 10) : 0
            property real ss: parts.length > 2 ? parseInt(parts[2], 10) : 0
            onPartsChanged: requestPaint()
            onWidthChanged: requestPaint()
            Accessible.ignored: true
            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                var r = Math.min(width, height) / 2 - 2;
                if (r <= 0) return;  // hidden (digital face) or not laid out yet
                var cx = width / 2, cy = height / 2;
                ctx.strokeStyle = Theme.secondary;
                ctx.lineWidth = 2;
                ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI); ctx.stroke();
                for (var i = 0; i < 12; i++) {
                    var a = i * Math.PI / 6;
                    ctx.lineWidth = i % 3 === 0 ? 3 : 1.5;
                    ctx.beginPath();
                    ctx.moveTo(cx + Math.sin(a) * r * 0.86, cy - Math.cos(a) * r * 0.86);
                    ctx.lineTo(cx + Math.sin(a) * r * 0.97, cy - Math.cos(a) * r * 0.97);
                    ctx.stroke();
                }
                function hand(angle, len, w, color) {
                    ctx.strokeStyle = color; ctx.lineWidth = w; ctx.lineCap = "round";
                    ctx.beginPath(); ctx.moveTo(cx, cy);
                    ctx.lineTo(cx + Math.sin(angle) * r * len, cy - Math.cos(angle) * r * len);
                    ctx.stroke();
                }
                hand(((hh % 12) + mm / 60) * Math.PI / 6, 0.5, 5, Theme.ink);
                hand((mm + ss / 60) * Math.PI / 30, 0.8, 3, Theme.ink);
                hand(ss * Math.PI / 30, 0.88, 1.5, Theme.accent);
            }
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.Fit
            minimumPixelSize: Theme.fontValue
            Layout.preferredHeight: root.height * (dial.visible ? 0.2 : 0.38)
            text: station.utcTime
            color: Theme.ink
            font.pixelSize: root.height
            font.weight: Font.Light
            Accessible.name: "UTC time " + text
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: station.utcDate + " · UTC"
            color: Theme.secondary
            font.pixelSize: Theme.fontBody + 4
        }
        Text {
            Layout.fillWidth: true
            visible: station.deTime.length > 0
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.Fit
            minimumPixelSize: Theme.fontValue
            Layout.preferredHeight: root.height * 0.2
            text: station.deClock
            color: Theme.accent
            font.pixelSize: root.height
            font.weight: Font.Light
            Accessible.name: "Local time " + text
        }
        Text {
            Layout.fillWidth: true
            visible: station.deTime.length > 0
            horizontalAlignment: Text.AlignHCenter
            text: station.deDate + " · " + station.deZoneLabel
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: station.auxTime === station.lstTime ? station.auxTime : station.lstTime + "  ·  " + station.auxTime
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight
        }
        Item { Layout.fillHeight: true }
    }

    TapHandler { onTapped: root.closeRequested() }
    Accessible.role: Accessible.Button
    Accessible.name: "Big clock, tap to close"
    Accessible.onPressAction: root.closeRequested()
}
