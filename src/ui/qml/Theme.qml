pragma Singleton
import QtQuick

// Design tokens from design/dashboard.html (see docs/native-approved-design-gap.md).
// `scale` grows type and spacing on large screens (TVs) and stays 1 on a
// 1024x600 panel; Main.qml sets it from the window size.
QtObject {
    property bool dark: false
    property real scale: 1
    // Small panel (7-inch Pi, 800x480 to 1280x720): no header, tight spacing.
    property bool dense: false
    // Accent choice from Settings: 0 blue, 1 green, 2 amber, 3 red, 4 purple.
    property int accentStyle: 0
    property bool phone: false   // portrait phone: one column, one pane
    // Height the on-screen keyboard (--touch-keyboard) covers at the bottom of the window; 0 when hidden.
    property real keyboardInset: 0

    readonly property color canvas: dark ? "#22282c" : "#eeede9"
    readonly property color surface: dark ? "#2b3236" : "#fafaf7"
    readonly property color surfaceRaised: dark ? "#323a3f" : "#ffffff"
    readonly property color ink: dark ? "#e2e7e6" : "#303a42"
    readonly property color secondary: dark ? "#97a4a8" : "#637074"
    readonly property color divider: dark ? "#41494b" : "#e0e3df"
    readonly property color accent: dark ? ["#9cb6c5", "#8fc79a", "#e6c06b", "#ec8f86", "#b9a8dc"][accentStyle]
                                         : ["#487991", "#2f7d3f", "#8a5f00", "#b3362b", "#6c4fa0"][accentStyle]
    readonly property color accentInk: dark ? "#1d2427" : "#ffffff"
    readonly property color focus: dark ? "#c7dbe6" : "#2f6077"
    // Status colours always appear next to text, never as the only signal.
    readonly property color good: dark ? "#8fc79a" : "#2f7d3f"
    readonly property color warn: dark ? "#e6c06b" : "#9a6a00"
    readonly property color bad: dark ? "#ec8f86" : "#b3362b"

    readonly property real radius: (dense ? 8 : 14) * scale
    readonly property real gap: (dense ? 5 : 10) * scale
    readonly property real pad: (dense ? 7 : 14) * scale
    // Minimum touch target. iPhone/iPad never go below Apple's 44 pt (phones run at
    // scale 0.85); the Pi's 800x480 panel keeps its tuned, slightly smaller size.
    readonly property real touch: Qt.platform.os === "ios" ? Math.max(44, 44 * scale) : 44 * scale

    readonly property int fontCaption: Math.round(12 * scale)
    readonly property int fontBody: Math.round(14 * scale)
    readonly property int fontTitle: Math.round(15 * scale)
    readonly property int fontValue: Math.round(26 * scale)
    readonly property int fontClock: Math.round(40 * scale)

    function stateColor(state) {
        if (state === "live") return good
        if (state === "stale" || state === "loading") return warn
        if (state === "unconfigured") return secondary
        return bad
    }
    function stateLabel(state) {
        return ({ live: "Live", stale: "Stale", loading: "Loading", offline: "Offline",
                  error: "Error", unconfigured: "Set up" })[state] || state
    }
}
