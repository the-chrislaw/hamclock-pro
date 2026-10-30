pragma Singleton
import QtQuick
QtObject {
    property QtObject settings: QtObject {
        property var panes: ["w1", "w2"]
        property bool demoMode: false
        property var paneRotations: [0, 0]
        property var paneRotationSets: [[], []]
        property int setCalls: 0
        function setPane(i, id) { const p = panes.slice(); p[i] = id; panes = p; setCalls++; return "" }
    }
    property Registry registry: Registry {}
    function touch() {}
}
