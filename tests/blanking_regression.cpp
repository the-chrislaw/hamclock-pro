#include "ui/blanking_model.hpp"

#include <QCoreApplication>
#include <cstdio>

using namespace hamclock;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    BlankingConfig off;
    CHECK(blanking_config_valid(off));
    CHECK(!should_blank(off, QTime(3, 0), 100000));  // nothing configured: never blank

    BlankingConfig idle;
    idle.idle_minutes = 5;
    CHECK(!should_blank(idle, QTime(12, 0), 299));
    CHECK(should_blank(idle, QTime(12, 0), 300));
    CHECK(!should_blank(idle, QTime(12, 0), -50));  // negative idle is treated as 0

    BlankingConfig night;
    night.off_hour = 23;
    night.on_hour = 6;  // crosses midnight
    CHECK(in_off_window(night, QTime(23, 0)) && in_off_window(night, QTime(0, 30)) && in_off_window(night, QTime(5, 59)));
    CHECK(!in_off_window(night, QTime(6, 0)) && !in_off_window(night, QTime(22, 59)) && !in_off_window(night, QTime(12, 0)));
    CHECK(!should_blank(night, QTime(2, 0), kWindowWakeSecs - 1));  // a touch wakes it briefly
    CHECK(should_blank(night, QTime(2, 0), kWindowWakeSecs));
    CHECK(!should_blank(night, QTime(12, 0), 100000));

    BlankingConfig day;
    day.off_hour = 9;
    day.on_hour = 17;
    CHECK(in_off_window(day, QTime(9, 0)) && !in_off_window(day, QTime(17, 0)));
    BlankingConfig same;
    same.off_hour = 8;
    same.on_hour = 8;
    CHECK(!in_off_window(same, QTime(8, 0)));

    BlankingConfig bad;
    bad.off_hour = 5;  // half a window
    CHECK(!blanking_config_valid(bad));
    bad = {};
    bad.idle_minutes = 241;
    CHECK(!blanking_config_valid(bad));
    bad = {};
    bad.off_hour = 24;
    bad.on_hour = 1;
    CHECK(!blanking_config_valid(bad));

    BlankingModel model;
    model.setConfig(idle);
    CHECK(!model.blanked());
    model.evaluate(QTime(12, 0), 400);
    CHECK(model.blanked());
    model.touch();
    CHECK(!model.blanked());
    model.setConfig(bad);  // rejected: keeps the previous config
    model.evaluate(QTime(12, 0), 400);
    CHECK(model.blanked());

    if (failures) return 1;
    std::puts("blanking-regression OK");
    return 0;
}
