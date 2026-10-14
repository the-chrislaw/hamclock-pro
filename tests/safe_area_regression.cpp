// SafeArea: defaults to zero, sanitizes bad input, emits only on change.
#include "../src/platform/apple_safearea.hpp"
#include <QCoreApplication>
#include <iostream>
#include <limits>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    hamclock::SafeArea area;
    int changes = 0;
    QObject::connect(&area, &hamclock::SafeArea::changed, [&] { ++changes; });

    check(area.left() == 0 && area.top() == 0 && area.right() == 0 && area.bottom() == 0, "default zero");
    area.setInsets({0, 0, 0, 0});
    check(changes == 0, "no signal for unchanged zero");
    area.setInsets({0, 24, 0, 20});
    check(changes == 1 && area.top() == 24 && area.bottom() == 20, "insets applied");
    area.setInsets({0, 24, 0, 20});
    check(changes == 1, "no signal for same value");
    area.setInsets({-5, std::numeric_limits<qreal>::quiet_NaN(), 3, 20});
    check(area.left() == 0 && area.top() == 0 && area.right() == 3 && changes == 2, "negative/NaN become zero");
    area.attach(nullptr);
    check(changes == 2, "attach(nullptr) harmless");

    std::cout << (failures ? "FAILED\n" : "safe-area-regression: ok\n");
    return failures ? 1 : 0;
}
