import QtQuick
ListModel {
    id: reg
    property var models: ({})
    property int created: 0
    function model(id) { return models[id] || null }
    Component.onCompleted: {
        const names = ["Space weather", "Solar wind", "Bands", "DX cluster", "Weather", "Satellites"]
        const m = {}
        for (let i = 0; i < names.length; ++i) {
            const id = "w" + (i + 1)
            m[id] = Qt.createQmlObject('import QtQuick; QtObject { property string widgetId: "' + id + '"; property string title: "' + names[i]
                + '"; property string state: "live"; property string statusText: "Live"; property string ageText: "just now" }', reg)
            append({ widgetId: id, title: names[i], card: Qt.resolvedUrl("../StubCard.qml"), widgetModel: m[id] })
        }
        models = m
    }
}
