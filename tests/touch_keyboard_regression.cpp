// TouchKeyboard.qml (--touch-keyboard): shows for a focused text field, taps type into it
// (upper case for upper-case fields, Shift toggles), Backspace deletes, Hide leaves the field.
#include "../src/ui/key_injector.hpp"
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QtQml>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}

// Stands in for BlankingModel: the keyboard reports each tap as activity.
class FakeBlanking : public QObject {
    Q_OBJECT
public:
    int touches = 0;
    Q_INVOKABLE void touch() { ++touches; }
};

QQuickItem* find_key(QQuickItem* root, const QString& label) {
    if (!root) return nullptr;
    if (root->property("label").toString() == label && root->isVisible()) return root;
    for (auto* child : root->childItems())
        if (auto* found = find_key(child, label)) return found;
    return nullptr;
}

// Waits briefly for the key: the popup and its label bindings settle in the event loop.
bool tap(QQuickWindow* window, const QString& label) {
    QQuickItem* key = nullptr;
    for (int i = 0; i < 100 && !key; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        key = find_key(window->contentItem(), label);
    }
    if (!key) std::cerr << "no key " << qPrintable(label) << '\n';
    return key && QMetaObject::invokeMethod(key, "activated");
}
// A real press and release at the key's centre, delivered like a touch-screen tap.
bool click(QQuickWindow* window, const QString& label) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    auto* key = find_key(window->contentItem(), label);
    if (!key) return false;
    const QPointF at = key->mapToScene(QPointF(key->width() / 2, key->height() / 2));
    for (const auto type : {QEvent::MouseButtonPress, QEvent::MouseButtonRelease}) {
        QMouseEvent event(type, at, at, window->mapToGlobal(at), Qt::LeftButton,
                          type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(window, &event);
    }
    QCoreApplication::processEvents();
    return true;
}
} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");
    const QString qml_dir = QStringLiteral(HC_QML_DIR);
    qmlRegisterSingletonType(QUrl::fromLocalFile(qml_dir + "/Theme.qml"), "HamClock", 1, 0, "Theme");
    qmlRegisterType(QUrl::fromLocalFile(qml_dir + "/TouchKeyboard.qml"), "HamClock", 1, 0, "TouchKeyboard");

    hamclock::KeyInjector injector;
    FakeBlanking blanking;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("keyInjector", &injector);
    engine.rootContext()->setContextProperty("blanking", &blanking);
    engine.loadData("import QtQuick\nimport QtQuick.Controls.Basic\nimport HamClock\n"
                    "ApplicationWindow { width: 800; height: 480; visible: true\n"
                    "  property alias field: f\n  property alias keyboard: k\n"
                    "  TextField { id: f; inputMethodHints: Qt.ImhUppercaseOnly }\n"
                    "  Button { id: b; y: 100; text: \"not text\" }\n"
                    "  TouchKeyboard { id: k; target: activeFocusItem }\n"
                    "  function focusButton() { b.forceActiveFocus() }\n"
                    "  property alias search: s\n  property alias popup: p\n"
                    "  Popup { id: p; modal: true; closePolicy: Popup.CloseOnPressOutside; width: 300; height: 100\n"
                    "    TextField { id: s; width: 200 } }\n}");
    auto* window = engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    check(window != nullptr, "test window loads");
    if (!window) return 1;
    auto* field = window->property("field").value<QQuickItem*>();
    auto* keyboard = window->property("keyboard").value<QObject*>();
    const auto shown = [&] { QCoreApplication::processEvents(); return keyboard->property("visible").toBool(); };

    check(!shown(), "hidden with nothing focused");
    QMetaObject::invokeMethod(window, "focusButton");
    check(!shown(), "hidden for a focused non-text control");
    field->forceActiveFocus();
    check(shown(), "shown for a focused text field");
    const qreal key_bottom = keyboard->property("y").toReal() + keyboard->property("height").toReal();
    check(qAbs(key_bottom - window->height()) < 1, "docked at the bottom of the window");

    check(tap(window, "W") && tap(window, "1") && tap(window, "A") && tap(window, "W"), "letter and digit keys found");
    check(field->property("text").toString() == "W1AW", "upper-case field types capitals");
    check(tap(window, "/") && tap(window, "⇧") && tap(window, "p"), "shift toggles to lower case");
    check(field->property("text").toString() == "W1AW/p", "lower case after shift");
    check(tap(window, "⌫"), "backspace key found");
    check(field->property("text").toString() == "W1AW/", "backspace deletes");
    check(blanking.touches >= 7, "each tap counts as activity for screen blanking");

    check(tap(window, "Hide"), "hide key found");
    check(!field->hasActiveFocus() && !shown(), "hide leaves the field and closes the keyboard");

    // Time zone search: a modal popup that closes on outside presses. Keyboard taps must
    // type into its field and leave it open.
    auto* popup = window->property("popup").value<QObject*>();
    auto* search = window->property("search").value<QQuickItem*>();
    QMetaObject::invokeMethod(popup, "open");
    search->forceActiveFocus();
    check(shown(), "shown above a modal popup's field");
    check(click(window, "l") && click(window, "o"), "keys clicked over a modal popup");
    check(search->property("text").toString() == "lo", "clicks type into the popup's field");
    check(popup->property("opened").toBool(), "modal popup stays open while typing");

    std::cout << (failures ? "FAILED\n" : "touch-keyboard-regression: ok\n");
    return failures ? 1 : 0;
}

#include "touch_keyboard_regression.moc"
