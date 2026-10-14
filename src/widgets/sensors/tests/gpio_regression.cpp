// GPIO output interface: line parsing, null backend degrades, fake backend records. Offline.
#include "../../../platform/gpio.hpp"
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
}

int main() {
    using namespace hamclock::platform;
    check(parse_bcm_line("17") == 17 && parse_bcm_line(" BCM27 ") == 27 && parse_bcm_line("bcm2") == 2, "valid lines");
    check(!parse_bcm_line("1") && !parse_bcm_line("28") && !parse_bcm_line("") && !parse_bcm_line("x"), "invalid lines rejected");

    auto none = make_default_gpio_outputs();
    check(!none->available() && !none->set(17, true) && !none->lastDriven(17), "default backend is unavailable and drives nothing");

    FakeGpioOutputs fake;
    check(fake.available() && !fake.lastDriven(17), "fake starts unset");
    check(fake.set(17, true) && fake.lastDriven(17) == true, "fake records high");
    check(fake.set(17, false) && fake.lastDriven(17) == false, "fake records low");
    check(!fake.set(1, true) && !fake.lastDriven(1), "fake rejects invalid line");
    return failures ? 1 : 0;
}
