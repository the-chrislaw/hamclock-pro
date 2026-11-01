// StationCard at 800x480 with the longest realistic values (test station W1AW).
// 1. Nothing is cut off: the widest 12-hour clock, two-digit sun times, a long city with "(auto)".
// 2. The DE/DX blocks survive a clock tick. They were rebuilt on every change of their text, and until
//    the next layout pass the new, unwrapped lines ran past the card: the check that reports clipped
//    text caught that at random (CI: "Local 17:05 · London (auto)"), and a tap could lose its target.
#include <QGuiApplication>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QtQml>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const std::string& label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}

constexpr auto kStation = R"QML(import QtQml
QtObject {
    property bool configured: true
    property bool hasDx: true
    property bool bannerIsCallsign: true
    property string callsign: "W1AW"
    property string bannerText: "W1AW"
    property string utcTime: "12:05:01 AM"
    property string utcDate: "Sun 1 Nov 2026"
    property string deClock: "12:05:01 PM"
    property string deDate: "Sun 1 Nov 2026"
    property string deTime: "12:05 PM"
    property string deZoneLabel: "America/New_York · EDT"
    property string auxTime: "Day 305 of 365 · week 44"
    property string deGrid: "FN31pr"
    property string dxGrid: "GF05rj"
    property string deCoordinates: "41.73° N, 72.71° W"
    property string dxCoordinates: "34.60° S, 58.40° W"
    property string deSun: "Rise in 13h 51m · Set in 23h 27m"
    property string dxSun: "Rise in 12h 35m · Set in 22h 19m"
    property string dxLocal: "Local 12:05 PM · Buenos Aires (auto)"
    property string pathText: "Short path\n8484 KM\n159° TRUE"
    function bannerColor(style) { return "" }
    function bannerBackground(style) { return "" }
    function cycleBanner() {}
    function cycleAuxTime() {}
    function toggleSunAt() {}
    function toggleLongPath() {}
}
)QML";

void texts_in(QQuickItem* item, QList<QQuickItem*>& out) {
    if (!item || !item->isVisible()) return;
    if (item->property("contentWidth").isValid() && !item->property("text").toString().isEmpty()) out << item;
    for (auto* child : item->childItems()) texts_in(child, out);
}

// The same test the app's --screenshot check applies: painted text inside the card.
QStringList cut_off(QQuickItem* card) {
    constexpr double tol = 1.5;
    const QRectF bound = card->mapRectToScene(card->boundingRect());
    QList<QQuickItem*> texts;
    texts_in(card, texts);
    QStringList found;
    for (auto* t : texts) {
        if (t->property("truncated").toBool() && t->objectName() != "elideOk") { found << t->property("text").toString(); continue; }
        const QRectF r = t->mapRectToScene(QRectF(0, 0, t->property("contentWidth").toDouble(), t->property("contentHeight").toDouble()));
        if (r.right() > bound.right() + tol || r.bottom() > bound.bottom() + tol) found << t->property("text").toString();
    }
    return found;
}

QQuickItem* text_item(QQuickItem* card, const QString& starts) {
    QList<QQuickItem*> texts;
    texts_in(card, texts);
    for (auto* t : texts) if (t->property("text").toString().startsWith(starts)) return t;
    return nullptr;
}

void settle() {
    for (int i = 0; i < 30; ++i) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}
} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");
    const QString qml = QStringLiteral(HC_QML_DIR);
    qmlRegisterSingletonType(QUrl::fromLocalFile(qml + "/Theme.qml"), "HamClock", 1, 0, "Theme");
    for (const auto* type : {"Card", "AppButton", "StatusBadge", "StationCard"})
        qmlRegisterType(QUrl::fromLocalFile(qml + "/" + type + ".qml"), "HamClock", 1, 0, type);

    QQmlApplicationEngine engine;
    QQmlComponent stub(&engine);
    stub.setData(kStation, QUrl());
    QObject* station = stub.create();
    check(station != nullptr, "station stub loads: " + stub.errorString().toStdString());
    if (!station) return 1;
    station->setParent(&engine);
    engine.rootContext()->setContextProperty("station", station);
    QQmlComponent settings_stub(&engine);
    settings_stub.setData("import QtQml\nQtObject { property int bannerBackground: 0; property int bannerStyle: 0; property string loadError: \"\" }", QUrl());
    QObject* settings = settings_stub.create();
    check(settings != nullptr, "settings stub loads");
    if (!settings) return 1;
    settings->setParent(&engine);
    engine.rootContext()->setContextProperty("appSettings", settings);

    // The 800x480 panel's station column: Main.qml's scale and dense mode, 165 * 0.85 wide.
    engine.loadData("import QtQuick\nimport QtQuick.Controls.Basic\nimport HamClock\n"
                    "ApplicationWindow { width: 800; height: 480; visible: true\n"
                    "  Component.onCompleted: { Theme.scale = 0.85; Theme.dense = true }\n"
                    "  property alias card: c\n"
                    "  StationCard { id: c; x: 6; y: 6; width: 140; height: 468 }\n}");
    auto* window = engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    check(window != nullptr, "test window loads");
    if (!window) return 1;
    auto* card = window->property("card").value<QQuickItem*>();
    settle();

    for (const auto& text : cut_off(card)) check(false, "cut off with the longest values: " + text.toStdString());
    check(text_item(card, "12:05:01") != nullptr, "the clock shows");
    check(text_item(card, "Local 12:05 PM") != nullptr, "the DX local time shows");

    // A tick must keep the DE/DX lines: same items, still wrapped, before any layout pass runs.
    auto* sun = text_item(card, "☀ Rise in 13h");
    auto* local = text_item(card, "Local 12:05 PM");
    check(sun && local, "DE sun and DX local lines exist");
    QPointer<QQuickItem> sun_before(sun), local_before(local);
    station->setProperty("deSun", "Rise in 13h 50m · Set in 23h 26m");
    station->setProperty("dxLocal", "Local 12:06 PM · Buenos Aires (auto)");
    auto* sun_after = text_item(card, "☀ Rise in 13h 50m");
    auto* local_after = text_item(card, "Local 12:06 PM");
    check(sun_after && sun_after == sun_before.data(), "the DE sun line is the same item after a tick");
    check(local_after && local_after == local_before.data(), "the DX local line is the same item after a tick");
    for (const auto& text : cut_off(card)) check(false, "cut off right after a tick: " + text.toStdString());

    // No DX target: one block, and the second comes back when a target is set.
    station->setProperty("hasDx", false);
    settle();
    check(text_item(card, "Local 12:06 PM") == nullptr, "no DX block without a target");
    check(text_item(card, "☀ Rise in 13h 50m") != nullptr, "the DE block stays without a target");
    station->setProperty("hasDx", true);
    settle();
    check(text_item(card, "Local 12:06 PM") != nullptr, "the DX block returns with a target");

    std::cerr << (failures ? "station-card-regression: failures\n" : "station-card-regression: ok\n");
    return failures ? 1 : 0;
}
