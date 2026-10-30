// Native Mac menu bar (MacMenuBar.qml): the app menu gets About and Settings...
// through their roles, and Edit, View, Window and Help hold the standard commands
// with the Mac shortcuts. Edit commands reach the focused text field, and View and
// Settings send their requests to the page. Runs offscreen: it checks the menu
// model, not the system menu bar (that is checked by hand with System Events).
#include <QDir>
#include <QGuiApplication>
#include <QKeySequence>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlListReference>
#include <QUrl>
#include <QWindow>
#include <iostream>
#include <memory>

namespace {
int failures = 0;
void check(bool ok, const std::string& label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}

// QPlatformMenuItem::MenuRole values.
constexpr int kNoRole = 0;
constexpr int kAboutRole = 4;
constexpr int kApplicationSpecificRole = 2;

QList<QObject*> list(QObject* object, const char* property) {
    QList<QObject*> out;
    QQmlListReference ref(object, property);
    for (qsizetype i = 0; i < ref.count(); ++i) out << ref.at(i);
    return out;
}

QObject* menuTitled(QObject* bar, const QString& title) {
    for (auto* menu : list(bar, "menus"))
        if (menu->property("title").toString() == title) return menu;
    return nullptr;
}

QObject* itemNamed(QObject* menu, const QString& text) {
    if (!menu) return nullptr;
    for (auto* item : list(menu, "items"))
        if (item->property("text").toString() == text) return item;
    return nullptr;
}

QString shortcutOf(QObject* item) {
    const QVariant value = item ? item->property("shortcut") : QVariant();
    if (value.typeId() == QMetaType::QString) return value.toString();
    if (value.canConvert<int>() && value.typeId() != QMetaType::QKeySequence)
        return QKeySequence(static_cast<QKeySequence::StandardKey>(value.toInt())).toString(QKeySequence::PortableText);
    return value.value<QKeySequence>().toString(QKeySequence::PortableText);
}

QString standard(QKeySequence::StandardKey key) {
    return QKeySequence(key).toString(QKeySequence::PortableText);
}

void activate(QObject* item) {
    if (item) QMetaObject::invokeMethod(item, "activate");
}
} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationVersion("9.8.7");
    if (argc < 2) { std::cerr << "usage: mac-menu-bar-regression <src/ui/qml dir>\n"; return 2; }
    const QUrl dir = QUrl::fromLocalFile(QDir(QString::fromLocal8Bit(argv[1])).absolutePath() + "/");

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import QtQuick.Window
Window {
    id: w
    width: 320; height: 200; visible: true
    property int settingsRequests: 0
    property int pathToggles: 0
    property string chosen: ""
    property alias field: field
    property alias menus: menus
    TextInput { id: field; text: "W1AW"; focus: true; width: 200; height: 30 }
    MacMenuBar {
        id: menus
        appWindow: w
        appearance: "dark"
        onSettingsRequested: w.settingsRequests++
        onPathToggleRequested: w.pathToggles++
        onAppearanceRequested: function (value) { w.chosen = value }
    }
}
)", dir.resolved(QUrl("menu_harness.qml")));
    std::unique_ptr<QObject> window(component.create());
    if (!window) {
        std::cerr << qPrintable(component.errorString()) << '\n';
        return 1;
    }
    auto* menus = window->property("menus").value<QObject*>();
    auto* field = window->property("field").value<QObject*>();
    auto* bar = menus ? menus->property("menuBar").value<QObject*>() : nullptr;
    check(bar && field, "harness loads the menu bar");
    if (!bar || !field) return 1;
    check(bar->property("window").value<QWindow*>() == nullptr, "global menu bar, not tied to one window");

    // Menu order after the app menu, as in every Mac app.
    QStringList titles;
    for (auto* menu : list(bar, "menus")) titles << menu->property("title").toString();
    check(titles == QStringList({"Edit", "View", "Window", "Help"}), "menus are Edit, View, Window, Help: got " + titles.join(", ").toStdString());

    // App menu items: placed by role, so they appear under "HamClock Pro".
    auto* help = menuTitled(bar, "Help");
    auto* about = itemNamed(help, "About HamClock Pro");
    auto* settings = itemNamed(help, "Settings…");
    check(about && about->property("role").toInt() == kAboutRole, "About HamClock Pro has the About role");
    // Not the Preferences role: Qt would retitle it "Preferences...".
    check(settings && settings->property("role").toInt() == kApplicationSpecificRole, "Settings… goes to the app menu under its own name");
    check(shortcutOf(settings) == "Ctrl+,", "Settings… is Command-comma");
    auto* support = itemNamed(help, "HamClock Pro Help");
    check(support && support->property("role").toInt() == kNoRole, "Help menu has HamClock Pro Help");
    check(menus->property("supportUrl").toString() == "https://the-chrislaw.github.io/hamclock-pro/support/", "Help opens the support page");

    // About panel shows the application version (CMake project version in the app).
    auto* dialog = menus->property("aboutDialog").value<QObject*>();
    check(dialog && dialog->property("text").toString() == "HamClock Pro 9.8.7", "About shows the version");

    // Settings… opens Settings, and is disabled while an overlay covers the page.
    activate(settings);
    check(window->property("settingsRequests").toInt() == 1, "Settings… requests Settings");
    menus->setProperty("settingsAvailable", false);
    check(settings && !settings->property("enabled").toBool(), "Settings… disabled under an overlay");

    // Edit: standard Mac shortcuts; nothing gets a text-heuristic role that would move it.
    auto* edit = menuTitled(bar, "Edit");
    const std::pair<const char*, QKeySequence::StandardKey> edits[] = {
        {"Undo", QKeySequence::Undo}, {"Redo", QKeySequence::Redo}, {"Cut", QKeySequence::Cut},
        {"Copy", QKeySequence::Copy}, {"Paste", QKeySequence::Paste}, {"Select All", QKeySequence::SelectAll}};
    for (const auto& [text, key] : edits) {
        auto* item = itemNamed(edit, text);
        check(item && shortcutOf(item) == standard(key), std::string("Edit > ") + text + " has its standard shortcut");
        check(item && item->property("role").toInt() == kNoRole, std::string("Edit > ") + text + " stays in Edit");
    }

    // Edit commands act on the focused text field.
    if (auto* w = qobject_cast<QWindow*>(window.get())) { w->requestActivate(); }
    QMetaObject::invokeMethod(field, "forceActiveFocus");
    QCoreApplication::processEvents();
    activate(itemNamed(edit, "Select All"));
    check(field->property("selectedText").toString() == "W1AW", "Select All selects the field");
    activate(itemNamed(edit, "Cut"));
    check(field->property("text").toString().isEmpty(), "Cut empties the field");
    activate(itemNamed(edit, "Undo"));
    check(field->property("text").toString() == "W1AW", "Undo restores the field");
    QMetaObject::invokeMethod(field, "deselect");
    field->setProperty("cursorPosition", 4);
    activate(itemNamed(edit, "Paste"));
    check(field->property("text").toString() == "W1AWW1AW", "Paste inserts the cut text");

    // View: path, appearance.
    auto* view = menuTitled(bar, "View");
    // macOS adds Enter/Exit Full Screen (Control-Command-F) to the View menu itself.
    check(!itemNamed(view, "Enter Full Screen") && !itemNamed(view, "Exit Full Screen"), "no second Full Screen item");
    auto* path = itemNamed(view, "Show Path");
    check(path && !path->property("enabled").toBool(), "Show Path disabled without a station");
    menus->setProperty("pathAvailable", true);
    menus->setProperty("showPath", true);
    check(path && path->property("enabled").toBool() && path->property("text").toString() == "Hide Path", "Hide Path when the path shows");
    activate(path);
    check(window->property("pathToggles").toInt() == 1, "Hide Path toggles the path");

    QObject* appearance = nullptr;
    for (auto* item : list(view, "items"))
        if (auto* sub = item->property("subMenu").value<QObject*>(); sub && sub->property("title") == "Appearance") appearance = sub;
    check(appearance != nullptr, "View has an Appearance submenu");
    auto* light = itemNamed(appearance, "Light");
    auto* dark = itemNamed(appearance, "Dark");
    auto* system = itemNamed(appearance, "System");
    check(light && dark && system, "Appearance offers Light, Dark, System");
    if (light && dark && system) {
        check(dark->property("checked").toBool() && !light->property("checked").toBool(), "saved appearance is checked");
        activate(light);
        check(window->property("chosen").toString() == "light", "Light requests the light appearance");
        menus->setProperty("appearance", "system"); // e.g. changed in Settings
        check(system->property("checked").toBool() && !light->property("checked").toBool(), "check mark follows the saved appearance");
    }

    // Window: Minimize is Command-M; Zoom and Bring All to Front are present.
    auto* windowMenu = menuTitled(bar, "Window");
    auto* minimize = itemNamed(windowMenu, "Minimize");
    check(minimize && shortcutOf(minimize) == "Ctrl+M", "Minimize is Command-M");
    check(itemNamed(windowMenu, "Zoom") && itemNamed(windowMenu, "Bring All to Front"), "Window has Zoom and Bring All to Front");

    std::cout << (failures ? "FAILED\n" : "mac-menu-bar-regression: ok\n");
    return failures ? 1 : 0;
}
