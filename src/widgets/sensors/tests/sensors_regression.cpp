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

    const char* wl = "Inter-| sta-|   Quality        |   Discarded packets\n face | tus | link level noise |  nwid  crypt\n"
                     " wlan0: 0000   70.  -52.  -256        0      0\n";
    check(sensors::parse_wireless_dbm(wl).value_or(0) == -52, "parses wireless level");
    check(!sensors::parse_wireless_dbm("Inter-|\n face |\n"), "no interfaces -> absent");
    check(!sensors::parse_wireless_dbm("a\nb\n wlan0: 0000 70. 12. -256\n"), "implausible level rejected");
    check(!sensors::read_wifi_dbm("/nonexistent/wireless"), "missing wireless file -> absent");
    const double du = sensors::read_disk_used_fraction("/").value_or(-1);
    check(du >= 0 && du <= 1, "disk fraction sane");
    check(!sensors::read_disk_used_fraction("/nonexistent/dir"), "bad disk path -> absent");

    WidgetContext ctx;
    SensorsModel absent(ctx, "/nonexistent/thermal", "/nonexistent/wireless", "/nonexistent/dir");
    absent.refresh();
    check(absent.state() == "unconfigured" && !absent.hasCpuTemp() && absent.cpuTemp().isEmpty(), "absent hardware degrades cleanly");
    put(r + "/wireless", wl);
    SensorsModel present(ctx, r, r + "/wireless", "/");
    present.refresh();
    check(present.state() == "live" && present.cpuTemp() == "61 °C" && present.cpuNote() == "Normal", "present hardware live");
    check(present.wifi() == "-52 dBm" && present.hasDisk() && present.disk().endsWith("% used"), "wifi and disk shown");
    return failures ? 1 : 0;
}
