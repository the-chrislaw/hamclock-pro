#include "ui/timers.hpp"

#include <cstdio>

using namespace hamclock;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

int main() {
    Stopwatch sw;
    CHECK(sw.elapsed_ms(1000) == 0);
    sw.start(1000);
    CHECK(sw.elapsed_ms(3500) == 2500);
    sw.start(4000);  // second start while running is ignored
    CHECK(sw.elapsed_ms(5000) == 4000);
    sw.stop(5000);
    CHECK(sw.elapsed_ms(9000) == 4000);
    sw.start(10000);
    CHECK(sw.elapsed_ms(10500) == 4500);
    sw.reset();
    CHECK(!sw.running() && sw.elapsed_ms(20000) == 0);
    CHECK(sw.elapsed_ms(-5) == 0);

    Countdown cd;
    cd.start(0);  // nothing to count
    CHECK(!cd.running());
    cd.set_duration_ms(90000);
    cd.start(1000);
    CHECK(cd.remaining_ms(31000) == 60000);
    cd.set_duration_ms(5000);  // ignored while running
    CHECK(cd.duration_ms() == 90000);
    cd.stop(31000);
    CHECK(cd.remaining_ms(99999) == 60000 && !cd.expired(99999));
    cd.start(100000);
    CHECK(!cd.expired(159999) && cd.expired(160000) && cd.remaining_ms(200000) == 0);
    cd.reset();
    CHECK(cd.remaining_ms(0) == 90000);

    CHECK(format_stopwatch(0) == "0:00:00.0");
    CHECK(format_stopwatch(3723456) == "1:02:03.4");
    CHECK(format_stopwatch(-1) == "0:00:00.0");
    CHECK(format_countdown(0) == "0:00:00");
    CHECK(format_countdown(1) == "0:00:01");
    CHECK(format_countdown(60000) == "0:01:00");
    CHECK(format_countdown(3600000) == "1:00:00");
    return failures ? 1 : 0;
}
