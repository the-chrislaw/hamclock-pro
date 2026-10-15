// Host sensor parsing and sysfs scanning against a fake thermal tree. Offline.
#include "../sensors_model.hpp"
#include "../sensors_read.hpp"
#include "../../../platform/ntp_status.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cmath>
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

    check(!sensors::read_env_sensor("/nonexistent/iio").any(), "missing iio tree -> absent");
    QTemporaryDir iio;
    put(iio.path() + "/iio:device0/name", "ads1015\n");
    put(iio.path() + "/iio:device0/in_temp_input", "99000\n");
    put(iio.path() + "/iio:device1/name", "bme280\n");
    put(iio.path() + "/iio:device1/in_temp_input", "21500\n");
    put(iio.path() + "/iio:device1/in_pressure_input", "101.325\n");
    put(iio.path() + "/iio:device1/in_humidityrelative_input", "50000\n");
    const auto env = sensors::read_env_sensor(iio.path());
    check(env.temp_c && *env.temp_c == 21.5 && env.pressure_hpa && std::abs(*env.pressure_hpa - 1013.25) < 1e-6
              && env.humidity_pct && *env.humidity_pct == 50.0, "reads bme280 fields in hPa/C/%");
    const double dp = sensors::dew_point_c(21.5, 50).value_or(-99);
    check(dp > 10.0 && dp < 11.0, "dew point ~10.5 C at 21.5 C / 50 %");
    check(!sensors::dew_point_c(20, 0), "zero humidity -> no dew point");
    QTemporaryDir bad;
    put(bad.path() + "/iio:device0/name", "bmp280\n");
    put(bad.path() + "/iio:device0/in_temp_input", "x\n");
    put(bad.path() + "/iio:device0/in_pressure_input", "5.0\n");
    check(!sensors::read_env_sensor(bad.path()).any(), "implausible/garbage values rejected");

    {
        const auto good = platform::parse_chrony_tracking(
            "Reference ID    : A9FEA97B (time.example)\nStratum         : 3\nLeap status     : Normal\n"
            "Last offset     : -0.000412345 seconds\n");
        check(good && good->synchronized && good->stratum == 3 && good->offset_ms && std::abs(*good->offset_ms - 0.412345) < 1e-6,
              "chrony tracking parsed");
        check(good && platform::describe(*good) == "Synced, <1 ms, stratum 3 via time.example", "sub-ms offset, stratum and source described");
        const auto odd = platform::parse_chrony_tracking("Reference ID    : A9FEA97B (bad name!<b>)\nStratum         : 2\nLeap status     : Normal\n");
        check(odd && odd->source.isEmpty() && platform::describe(*odd) == "Synced, stratum 2", "unsafe source name dropped");
        const auto bad = platform::parse_chrony_tracking("Reference ID    : 00000000 ()\nStratum         : 0\nLeap status     : Not synchronised\n");
        check(bad && !bad->synchronized && !bad->offset_ms && platform::describe(*bad) == "Not synchronised", "unsynchronised chrony not live");
        check(!platform::parse_chrony_tracking("garbage"), "non-chrony text rejected");
        check(platform::parse_timedatectl("NTPSynchronized=yes\n")->synchronized, "timedatectl yes");
        check(!platform::parse_timedatectl("NTPSynchronized=no\n")->synchronized, "timedatectl no");
        check(!platform::parse_timedatectl("oops"), "timedatectl garbage rejected");
    }

    WidgetContext ctx;
    SensorsModel absent(ctx, "/nonexistent/thermal", "/nonexistent/wireless", "/nonexistent/dir", "/nonexistent/iio", nullptr);
    absent.refresh();
    check(absent.state() == "unconfigured" && !absent.hasCpuTemp() && absent.cpuTemp().isEmpty(), "absent hardware degrades cleanly");
    put(r + "/wireless", wl);
    SensorsModel present(ctx, r, r + "/wireless", "/", "/nonexistent/iio", nullptr);
    present.refresh();
    check(present.state() == "live" && present.cpuTemp() == "61 °C" && present.cpuNote() == "Normal", "present hardware live");
    check(present.wifi() == "-52 dBm" && present.hasDisk() && present.disk().endsWith("% used"), "wifi and disk shown");
    SensorsModel envOnly(ctx, "/nonexistent/thermal", "/nonexistent/wireless", "/nonexistent/dir", iio.path(), nullptr);
    envOnly.refresh();
    check(envOnly.state() == "live" && envOnly.envTemp() == "21.5 °C" && envOnly.envPressure() == "1013 hPa"
              && envOnly.envHumidity() == "50%" && !envOnly.envDewPoint().isEmpty(), "env sensor shown");
    {
        sensors::EnvReading r{21.5, 1013.0, 50.0};
        sensors::apply_offsets(r, "-1.5", " 3 ");
        check(std::abs(*r.temp_c - 20.0) < 1e-9 && std::abs(*r.pressure_hpa - 1016.0) < 1e-9, "offsets applied");
        sensors::EnvReading q{21.5, 1013.0, 50.0};
        sensors::apply_offsets(q, "", "abc");
        check(*q.temp_c == 21.5 && *q.pressure_hpa == 1013.0, "empty/garbage offsets ignored");
        sensors::apply_offsets(q, "99", "-500");
        check(*q.temp_c == 21.5 && *q.pressure_hpa == 1013.0, "implausible offsets ignored");
        sensors::EnvReading none;
        sensors::apply_offsets(none, "1", "1");
        check(!none.any(), "absent readings stay absent");
        QTemporaryDir cfg;
        AppSettings settings(cfg.filePath("station.conf"), cfg.path());
        put(settings.optionsPath(), "[sensors]\ntemp_offset_c=-1.5\npressure_offset_hpa=3\n");
        WidgetContext c2;
        c2.settings = &settings;
        SensorsModel cal(c2, "/nonexistent/thermal", "/nonexistent/wireless", "/nonexistent/dir", iio.path(), nullptr);
        cal.refresh();
        check(cal.envTemp() == "20.0 °C" && cal.envPressure() == "1016 hPa", "model applies options.ini offsets");
    }
    SensorsModel clockOnly(ctx, "/nonexistent/thermal", "/nonexistent/wireless", "/nonexistent/dir", "/nonexistent/iio",
                           [] { return std::optional<platform::ClockSync>(platform::ClockSync{true, 12.4, 2, {}}); });
    clockOnly.refresh();
    check(clockOnly.state() == "live" && clockOnly.hasClock() && clockOnly.clock() == "Synced, 12 ms, stratum 2", "clock shown");
    {
        sensors::PressureHistory h;
        check(h.trend_word().isEmpty(), "no history -> no trend");
        h.add(0, 1013.0);
        h.add(10, 1000.0);  // within a minute: ignored
        h.add(20 * 60, 1013.0);
        check(h.trend_word().isEmpty(), "under 30 min -> no trend");
        h.add(60 * 60, 1011.0);
        check(h.trend_word() == "Falling" && std::abs(*h.change_per_3h() + 6.0) < 1e-9, "falling scaled to 3 h");
        sensors::PressureHistory s;
        s.add(0, 1010.0); s.add(3600, 1010.2);
        check(s.trend_word() == "Steady", "small change steady");
        s.add(2 * 3600, 1013.0);
        check(s.trend_word() == "Rising", "rising");
        s.add(5 * 3600, 1013.0);  // old samples trimmed past 3 h
        check(s.change_per_3h() && std::abs(*s.change_per_3h()) < 1e-9, "window trimmed to 3 h");
    }
    {
        sensors::PeakHistory p;
        check(!p.peak(), "no samples -> no peak");
        p.add(0, 60.0); p.add(10, 99.0);  // within a minute: ignored
        p.add(120, 71.5); p.add(240, 65.0);
        check(p.peak() && *p.peak() == 71.5, "peak is the maximum");
        p.add(4 * 3600, 50.0);  // older samples leave the 3 h window
        check(p.peak() && *p.peak() == 50.0, "peak window trimmed to 3 h");
    }
    {
        sensors::PeakHistory w;
        check(!w.trough(), "no samples -> no trough");
        w.add(0, -60.0); w.add(120, -82.0); w.add(240, -70.0);
        check(w.trough() && *w.trough() == -82.0, "trough is the minimum");
    }
    return failures ? 1 : 0;
}
