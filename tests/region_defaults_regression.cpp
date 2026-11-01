// Units, 12/24 h, date order and week start follow the device region on a new install only.
#include "ui/app_settings.hpp"
#include "ui/region_defaults.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QLocale>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

using hamclock::AppSettings;
using hamclock::RegionDefaults;
using hamclock::region_defaults;

namespace {
void write_station(const QString& path) {
    QFile file(path);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write("version=1\ncallsign=W1AW\nde_lat=41.7292\nde_lon=-72.7083\ndx_lat=35.68\ndx_lon=139.69\nappearance=system\n");
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // Pure mapping from a locale.
    const RegionDefaults us = region_defaults(QLocale("en_US"));
    CHECK(us.imperial && us.clock12h && us.date_order == 1 && us.week_starts_sunday);
    const RegionDefaults de = region_defaults(QLocale("de_DE"));
    CHECK(!de.imperial && !de.clock12h && de.date_order == 0 && !de.week_starts_sunday);
    const RegionDefaults gb = region_defaults(QLocale("en_GB"));   // Pi OS default: km, 24 h, Monday
    CHECK(!gb.imperial && !gb.clock12h && gb.date_order == 0 && !gb.week_starts_sunday);
    const RegionDefaults jp = region_defaults(QLocale("ja_JP"));   // year first
    CHECK(!jp.imperial && jp.date_order == 2);

    // New install (no station, no options): the region's choices are used and written down,
    // so they survive once the station is saved.
    {
        QLocale::setDefault(QLocale("en_US"));
        QTemporaryDir dir;
        {
            AppSettings settings(dir.filePath("station.conf"), dir.path());
            CHECK(settings.imperialUnits());
            CHECK(settings.clock12h());
            CHECK(settings.dateOrder() == 1);
            CHECK(settings.weekStartsSunday());
            CHECK(settings.saveStation("W1AW", 41.7292, -72.7083, 35.68, 139.69, "America/New_York").isEmpty());
        }
        QLocale::setDefault(QLocale("de_DE"));
        AppSettings again(dir.filePath("station.conf"), dir.path());
        CHECK(again.imperialUnits() && again.clock12h() && again.dateOrder() == 1 && again.weekStartsSunday());
    }

    // Existing install (station saved, choices never touched): nothing changes under it.
    {
        QLocale::setDefault(QLocale("en_US"));
        QTemporaryDir dir;
        write_station(dir.filePath("station.conf"));
        AppSettings settings(dir.filePath("station.conf"), dir.path());
        CHECK(!settings.imperialUnits());
        CHECK(!settings.clock12h());
        CHECK(settings.dateOrder() == 0);
        CHECK(!settings.weekStartsSunday());
        CHECK(!QSettings(dir.filePath("options.ini"), QSettings::IniFormat).contains("units/imperial"));
    }

    // A choice already made on a new install is kept.
    {
        QLocale::setDefault(QLocale("en_US"));
        QTemporaryDir dir;
        {
            QSettings ini(dir.filePath("options.ini"), QSettings::IniFormat);
            ini.setValue("units/imperial", false);
        }
        AppSettings settings(dir.filePath("station.conf"), dir.path());
        CHECK(!settings.imperialUnits());
        CHECK(settings.clock12h());   // untouched keys still follow the region
    }

    std::printf(g_fail ? "region-defaults-regression: %d failure(s)\n" : "region-defaults-regression: ok\n", g_fail);
    return g_fail ? 1 : 0;
}
