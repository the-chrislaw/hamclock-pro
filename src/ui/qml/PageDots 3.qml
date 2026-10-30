import QtQuick
import HamClock
import "pager.mjs" as Pager

// Phone page dots under a swipeable pane: a window of up to seven around the
// current page. `progress` (-1..1) is how far a swipe has gone towards the
// previous/next page; the current dot shrinks and the next one grows with it.
// Passive: taps and swipes go through to the pane.
Row {
    id: dots
    objectName: "pageDots"
    property var pages: []
    property string current: ""
    property real progress: 0
    readonly property int windowSize: 7
    readonly property int at: Math.max(0, pages.indexOf(current))
    readonly property int first: Math.max(0, Math.min(pages.length - windowSize, at - 3))
    readonly property real position: at + progress
    spacing: 5 * Theme.scale

    Repeater {
        model: Math.min(dots.windowSize, dots.pages.length)
        Rectangle {
            readonly property real weight: Pager.dotWeight(dots.first + index, dots.position)
            width: (5 + 3 * weight) * Theme.scale
            height: width
            radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            color: Qt.tint(Theme.secondary, Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, weight))
            opacity: 0.5 + 0.5 * weight
        }
    }
}
