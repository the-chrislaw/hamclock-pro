// Station file that cannot be read: the reason reaches the UI (loadError property),
// and the first save keeps the unreadable file beside the new one instead of losing it.
#include "ui/app_settings.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

namespace {
void write(const QString& path, const QByteArray& bytes) {
    QFile file(path);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(bytes);
}
QByteArray read(const QString& path) {
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    return file.readAll();
}
} // namespace

using hamclock::AppSettings;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // A file written without the appearance line (hand-edited or older) loads as "system".
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("station.conf");
        write(path, "version=1\ncallsign=W1AW\nde_lat=41.7292\nde_lon=-72.7083\ndx_lat=35.68\ndx_lon=139.69\n");
        AppSettings settings(path, dir.path());
        CHECK(settings.configured());
        CHECK(settings.callsign() == "W1AW");
        CHECK(settings.appearance() == "system");
        CHECK(settings.property("loadError").toString().isEmpty());
    }

    // A file that really can't be read: not configured, the reason is a QML property.
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("station.conf");
        const QByteArray broken = "version=1\ncallsign=W1AW\nde_lat=north\n";
        write(path, broken);
        AppSettings settings(path, dir.path());
        CHECK(!settings.configured());
        const QString error = settings.property("loadError").toString();
        CHECK(error.contains("could not be read"));

        // Saving keeps the unreadable file as station.conf.unreadable and clears the error.
        CHECK(settings.saveStation("W1AW", 41.7292, -72.7083, 35.68, 139.69, "America/New_York").isEmpty());
        CHECK(settings.configured());
        CHECK(settings.property("loadError").toString().isEmpty());
        CHECK(read(path + ".unreadable") == broken);
        CHECK(read(path).contains("callsign=W1AW"));

        // A later save does not touch the kept copy again.
        CHECK(settings.saveStation("W1AW", 41.0, -72.0, 35.68, 139.69, "America/New_York").isEmpty());
        CHECK(read(path + ".unreadable") == broken);
    }

    // A readable file is never copied aside.
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("station.conf");
        AppSettings settings(path, dir.path());
        CHECK(settings.property("loadError").toString().isEmpty());
        CHECK(settings.saveStation("W1AW", 41.7292, -72.7083, 35.68, 139.69, "").isEmpty());
        CHECK(!QFile::exists(path + ".unreadable"));
    }

    std::printf(g_fail ? "station-file-regression: %d failure(s)\n" : "station-file-regression: ok\n", g_fail);
    return g_fail ? 1 : 0;
}
