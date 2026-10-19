// TextEntryHint: silent by default, explains the iPad-app-on-Mac case, emits only on change.
#include "../src/platform/apple_textentry.hpp"
#include <QCoreApplication>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    hamclock::TextEntryHint hint;
    int changes = 0;
    QObject::connect(&hint, &hamclock::TextEntryHint::changed, [&] { ++changes; });

    check(hint.message().isEmpty() && !hint.hardwareKeyboard() && !hint.iosAppOnMac(), "default silent");
    hint.setState(false, false);
    check(changes == 0, "no signal for unchanged state");
    hint.setState(true, false);
    check(changes == 1 && hint.hardwareKeyboard() && hint.message().isEmpty(), "hardware keyboard on iPad is silent");
    hint.setState(true, true);
    check(changes == 2 && !hint.message().isEmpty(), "iPad app on Mac shows a message");
    hint.setState(true, true);
    check(changes == 2, "no signal for same state");
    hint.refresh();
    check(changes == 2 || changes == 3, "refresh harmless off iOS");

    std::cout << (failures ? "FAILED\n" : "text-entry-hint-regression: ok\n");
    return failures ? 1 : 0;
}
