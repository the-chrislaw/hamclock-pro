// Host sensor parsing and sysfs scanning against a fake thermal tree. Offline.
#include "../sensors_model.hpp"
#include "../sensors_read.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
void put(const QString& path, const char* text) {
    QDir().mkpath(QFileInfo(path).path());
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(text);
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    check(sensors::parse_millidegrees("48312\n").value_or(0) > 48.3, "parses millidegrees");
    check(!sensors::parse_millidegrees(""), "empty rejected");
    check(!sensors::parse_millidegrees("abc"), "garbage rejected");
    check(!sensors::parse_millidegrees("-127000"), "implausible rejected");

    check(!sensors::read_cpu_temp("/nonexistent/thermal"), "missing tree -> absent");

    QTemporaryDir dir;
    const QString r = dir.path();
    put(r + "/thermal_zone0/type", "acpitz\n");
    put(r + "/thermal_zone0/temp", "30000\n");
    put(r + "/thermal_zone1/type", "x86_pkg_temp\n");
    put(r + "/thermal_zone1/temp", "61000\n");
    check(sensors::read_cpu_temp(r).value_or(0) == 61.0, "prefers CPU-typed zone");

    QTemporaryDir only;
    put(only.path() + "/thermal_zone0/type", "battery\n");
    put(only.path() + "/thermal_zone0/temp", "bad\n");
    put(only.path() + "/thermal_zone1/temp", "40000\n");
    check(sensors::read_cpu_temp(only.path()).value_or(0) == 40.0, "skips invalid, falls back to first valid");

    WidgetContext ctx;
    SensorsModel absent(ctx, "/nonexistent/thermal");
    absent.refresh();
    check(absent.state() == "unconfigured" && !absent.hasCpuTemp() && absent.cpuTemp().isEmpty(), "absent hardware degrades cleanly");
    SensorsModel present(ctx, r);
    present.refresh();
    check(present.state() == "live" && present.cpuTemp() == "61 °C" && present.cpuNote() == "Normal", "present hardware live");
    return failures ? 1 : 0;
}
