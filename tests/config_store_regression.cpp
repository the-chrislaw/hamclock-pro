// Saved configurations: names, save/restore/rename/delete against a temp dir.
#include <cstdio>
#include <QFile>
#include <QTemporaryDir>

#include "ui/config_store.hpp"

using namespace hamclock;

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

static void write(const QString& p, const QByteArray& b) { QFile f(p); f.open(QIODevice::WriteOnly); f.write(b); }
static QByteArray read(const QString& p) { QFile f(p); f.open(QIODevice::ReadOnly); return f.readAll(); }

int main() {
    CHECK(config_clean_name("  Contest  night ") == "Contest night");
    CHECK(config_clean_name("").isEmpty());
    CHECK(config_clean_name("../x").isEmpty());
    CHECK(config_clean_name("a/b").isEmpty());
    CHECK(config_clean_name(QString(25, 'a')).isEmpty());
    CHECK(config_clean_name(QString(24, 'a')).size() == 24);

    QTemporaryDir tmp;
    const QString root = tmp.path();
    CHECK(config_list(root).isEmpty());
    CHECK(config_save(root, "bad/name") != "");

    write(root + "/options.ini", "[banner]\nstyle=2\n");
    CHECK(config_save(root, "Home").isEmpty());
    CHECK(config_list(root) == QStringList{"Home"});
    write(root + "/options.ini", "[banner]\nstyle=4\n");
    CHECK(config_save(root, "Contest").isEmpty());
    CHECK((config_list(root) == QStringList{"Contest", "Home"}));
    CHECK(config_restore(root, "Home").isEmpty());
    CHECK(read(root + "/options.ini") == "[banner]\nstyle=2\n");
    CHECK(config_save(root, "Home").isEmpty());  // overwrite allowed, no growth
    CHECK(config_list(root).size() == 2);

    CHECK(config_rename(root, "Contest", "Home") == "That name is taken.");
    CHECK(config_rename(root, "Contest", "Field Day").isEmpty());
    CHECK((config_list(root) == QStringList{"Field Day", "Home"}));
    CHECK(config_rename(root, "Nope", "X") == "No such configuration.");
    CHECK(config_restore(root, "Nope") == "No such configuration.");
    CHECK(config_delete(root, "Field Day").isEmpty());
    CHECK(config_delete(root, "Field Day") == "No such configuration.");

    for (int i = 0; i < kMaxConfigs; ++i) config_save(root, QString("c%1").arg(i));
    CHECK(config_list(root).size() == kMaxConfigs);
    CHECK(config_save(root, "one more") == "At most 12 saved configurations.");

    QTemporaryDir bare;  // no options.ini yet: empty snapshot restores defaults
    CHECK(config_save(bare.path(), "Fresh").isEmpty());
    write(bare.path() + "/options.ini", "x=1\n");
    CHECK(config_restore(bare.path(), "Fresh").isEmpty());
    CHECK(read(bare.path() + "/options.ini").isEmpty());

    if (g_fail) return 1;
    std::puts("config-store-regression: ok");
    return 0;
}
