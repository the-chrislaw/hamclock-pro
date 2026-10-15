import QtQuick
import QtQuick.Layouts
import HamClock

// Stopwatch and countdown, opened by long-pressing the station clock.
// Fits one screen without scrolling; the two timers sit side by side on wide windows.
Item {
    id: root
    signal closeRequested()
    property int alarmMinutes: 7 * 60

    GridLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: alarmArea.top
        anchors.margins: Theme.pad
        columns: width > 560 * Theme.scale ? 2 : 1
        columnSpacing: Theme.gap
        rowSpacing: Theme.gap

        Card {
            title: "Stopwatch"
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                width: parent.width
                spacing: Theme.gap
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: Theme.fontValue
                    text: timers.stopwatchText
                    color: Theme.ink
                    font.pixelSize: Theme.fontClock * 1.4
                    font.weight: Font.Light
                    Accessible.name: "Stopwatch " + text
                }
                RowLayout {
                    Layout.fillWidth: true
                    AppButton { Layout.fillWidth: true; primary: true; text: timers.stopwatchRunning ? "Stop" : "Start"; onClicked: timers.toggleStopwatch() }
                    AppButton { Layout.fillWidth: true; text: "Reset"; onClicked: timers.resetStopwatch() }
                }
            }
        }

        Card {
            title: "Countdown"
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                width: parent.width
                spacing: Theme.gap
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: Theme.fontValue
                    text: timers.countdownExpired ? "Time!" : timers.countdownText
                    color: timers.countdownExpired ? Theme.accent : Theme.ink
                    font.pixelSize: Theme.fontClock * 1.4
                    font.weight: Font.Light
                    Accessible.name: "Countdown " + text
                }
                RowLayout {
                    Layout.fillWidth: true
                    enabled: !timers.countdownRunning
                    AppButton { Layout.fillWidth: true; text: "+1 min"; onClicked: timers.addCountdownMinutes(1) }
                    AppButton { Layout.fillWidth: true; text: "+10 min"; onClicked: timers.addCountdownMinutes(10) }
                    AppButton { Layout.fillWidth: true; text: "-1 min"; onClicked: timers.addCountdownMinutes(-1) }
                }
                RowLayout {
                    Layout.fillWidth: true
                    AppButton { Layout.fillWidth: true; primary: true; text: timers.countdownRunning ? "Pause" : "Start"; onClicked: timers.toggleCountdown() }
                    AppButton { Layout.fillWidth: true; text: "Reset"; onClicked: timers.resetCountdown() }
                }
            }
        }
    }

    Column {
      id: alarmArea
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      anchors.margins: Theme.pad
      spacing: Theme.gap
      Flow {
        width: parent.width
        spacing: Theme.gap
        Repeater {
            model: alarms.items
            AppButton { text: modelData; onClicked: alarms.remove(index); Accessible.description: "Tap to delete" }
        }
      }
      Row {
        spacing: Theme.gap
        Text { anchors.verticalCenter: parent.verticalCenter; text: "Alarm (UTC)"; color: Theme.secondary; font.pixelSize: Theme.fontBody }
        AppButton { text: "-"; onClicked: alarmMinutes = (alarmMinutes + 1440 - 15) % 1440 }
        Text { anchors.verticalCenter: parent.verticalCenter; text: Qt.formatTime(new Date(2000, 0, 1, Math.floor(alarmMinutes / 60), alarmMinutes % 60), "HH:mm"); color: Theme.ink; font.pixelSize: Theme.fontBody + 4 }
        AppButton { text: "+"; onClicked: alarmMinutes = (alarmMinutes + 15) % 1440 }
        AppButton { text: "Daily"; onClicked: alarms.add(Math.floor(alarmMinutes / 60), alarmMinutes % 60, true) }
        AppButton { text: "Once"; onClicked: alarms.add(Math.floor(alarmMinutes / 60), alarmMinutes % 60, false) }
        Text { anchors.verticalCenter: parent.verticalCenter; visible: alarms.items.length > 0; text: "tap an alarm to delete"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
      }
    }

    AppButton {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.pad
        text: "Close"
        onClicked: root.closeRequested()
    }
}
