import QtQuick
import QtQuick.Layouts
import HamClock

// Stopwatch and countdown, opened by long-pressing the station clock.
// Fits one screen without scrolling; the two timers sit side by side on wide windows.
Item {
    id: root
    signal closeRequested()

    GridLayout {
        anchors.fill: parent
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

    AppButton {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.pad
        text: "Close"
        onClicked: root.closeRequested()
    }
}
