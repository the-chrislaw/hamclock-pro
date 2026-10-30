// KeyInjector (on-screen keyboard backend): taps arrive as real key events, so a
// text field sees ordinary typing (textEdited fires, Return accepts, Backspace deletes).
#include "../src/ui/key_injector.hpp"
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <iostream>
#include <memory>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}

std::unique_ptr<QQuickItem> make_field(QQmlEngine& engine) {
    QQmlComponent component(&engine);
    component.setData("import QtQuick\n"
                      "TextInput { property int edits: 0; property int accepts: 0\n"
                      "  onTextEdited: edits++; onAccepted: accepts++ }", QUrl());
    auto* item = qobject_cast<QQuickItem*>(component.create());
    if (!item) std::cerr << qPrintable(component.errorString());
    return std::unique_ptr<QQuickItem>(item);
}
} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    QQmlEngine engine;
    hamclock::KeyInjector keys;

    auto field = make_field(engine);
    check(field != nullptr, "test field created");
    if (!field) return 1;
    field->setFocus(true);

    check(keys.type(field.get(), "W1AW"), "type reports delivery");
    check(field->property("text").toString() == "W1AW", "typed text reaches the field");
    check(field->property("edits").toInt() == 4, "each key is a user edit (textEdited)");

    check(keys.type(field.get(), "/p fn31"), "type punctuation, space and lowercase");
    check(field->property("text").toString() == "W1AW/p fn31", "punctuation, space and case kept");

    check(keys.press(field.get(), Qt::Key_Backspace), "backspace delivered");
    check(field->property("text").toString() == "W1AW/p fn3", "backspace deletes one character");

    keys.press(field.get(), Qt::Key_Left);
    keys.type(field.get(), "X");
    check(field->property("text").toString() == "W1AW/p fnX3", "left arrow moves the cursor");

    check(keys.press(field.get(), Qt::Key_Return), "return delivered");
    check(field->property("accepts").toInt() == 1, "return accepts the field");

    check(!keys.type(nullptr, "A"), "no target: nothing typed, reports false");
    check(!keys.press(nullptr, Qt::Key_Backspace), "no target: press reports false");

    std::cout << (failures ? "FAILED\n" : "key-injector-regression: ok\n");
    return failures ? 1 : 0;
}
