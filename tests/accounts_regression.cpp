// Settings > Accounts: one sign-in per service, and nothing an owner saved before 1.0 is lost.
// Covers the pre-1.0 single lookup account (lookup/service + lookup/user + lookup/password,
// in the file on the Pi or in the Keychain on Apple devices) moving to its own service, and
// that passwords never reach options.ini or saved configurations. Offline; in-memory store.
#include "../src/platform/credential_store.hpp"
#include "../src/ui/app_settings.hpp"
#include "../src/ui/config_store.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <iostream>
#include <memory>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}

struct MemoryStore final : hamclock::CredentialStore {
    std::shared_ptr<QHash<QString, QString>> values = std::make_shared<QHash<QString, QString>>();
    bool set(const QString& k, const QString& v) override { values->insert(k, v); return true; }
    QString get(const QString& k) const override { return values->value(k); }
    bool remove(const QString& k) override { values->remove(k); return true; }
};

// The store key AppSettings uses for `name` in settings directory `dir`.
QString store_key(const QString& name, const QString& dir) {
    const auto path = QFileInfo(dir).absoluteFilePath().toUtf8();
    return name + "@" + QCryptographicHash::hash(path, QCryptographicHash::Sha1).toHex().left(16);
}

QString file_text(const QString& path) {
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}

void write_old_credentials(const QString& dir, const QString& service, bool with_password) {
    QSettings ini(dir + "/credentials.ini", QSettings::IniFormat);
    ini.setValue("lookup/service", service);
    ini.setValue("lookup/user", "W1AW");
    if (with_password) ini.setValue("lookup/password", "lookpw");
    ini.setValue("hamalert/user", "W1AW");
    if (with_password) ini.setValue("hamalert/password", "alertpw");
    ini.sync();
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;

    // Pi: the old account in the owner-only file moves to its service and stays there.
    {
        QTemporaryDir dir;
        write_old_credentials(dir.path(), "hamqth", true);
        {
            AppSettings settings({}, dir.path());
            check(settings.hamqthUser() == "W1AW" && settings.accountSignedIn("hamqth") && settings.lookupService() == "hamqth" &&
                      settings.lookupPassword() == "lookpw",
                  "pi: old HamQTH account kept as the HamQTH sign-in");
            check(settings.hamAlertUser() == "W1AW" && settings.hamAlertPassword() == "alertpw", "pi: HamAlert login kept");
            check(!settings.accountSignedIn("qrz"), "pi: QRZ.com not signed in");
        }
        const auto text = file_text(dir.filePath("credentials.ini"));
        check(!text.contains("lookup") && text.contains("hamqth") && text.contains("lookpw"),
              "pi: file rewritten in the new form, password still only there");
        AppSettings again({}, dir.path());
        check(again.hamqthUser() == "W1AW" && again.lookupPassword() == "lookpw", "pi: survives a restart");
    }

    // Mac/iPhone/iPad: user names in the file, passwords in the Keychain under the old key.
    {
        QTemporaryDir dir;
        write_old_credentials(dir.path(), "qrz", false);
        auto store = std::make_unique<MemoryStore>();
        auto values = store->values;
        values->insert(store_key("lookup.password", dir.path()), "lookpw");
        values->insert(store_key("hamalert.password", dir.path()), "alertpw");
        AppSettings settings({}, dir.path());
        // Before the Keychain is attached (main.cpp order) nothing may be rewritten or dropped.
        check(file_text(dir.filePath("credentials.ini")).contains("lookup"), "keychain: first read leaves the old entries alone");
        settings.setSecretStore(std::move(store));
        check(settings.qrzUser() == "W1AW" && settings.accountSignedIn("qrz") && settings.lookupService() == "qrz" &&
                  settings.lookupPassword() == "lookpw",
              "keychain: old QRZ.com account kept as the QRZ.com sign-in");
        check(settings.hamAlertUser() == "W1AW" && settings.hamAlertPassword() == "alertpw", "keychain: HamAlert login kept");
        check(values->value(store_key("qrz.password", dir.path())) == "lookpw" &&
                  !values->contains(store_key("lookup.password", dir.path())),
              "keychain: password moved to the QRZ.com entry");
        const auto text = file_text(dir.filePath("credentials.ini"));
        check(!text.contains("lookup") && !text.contains("lookpw") && !text.contains("alertpw"), "keychain: no password in the file");
    }

    // Sign in / out per service; QRZ.com is preferred for lookups.
    {
        QTemporaryDir dir;
        AppSettings settings({}, dir.path());
        check(settings.lookupService() == "none" && !settings.accountSignedIn("hamqth"), "fresh: nothing signed in");
        check(!settings.signIn("hamqth", "", "pw").isEmpty(), "user name required");
        check(!settings.signIn("hamqth", "W1AW", "").isEmpty(), "password required the first time");
        check(!settings.signIn("dropbox", "W1AW", "pw").isEmpty(), "unknown service refused");
        check(settings.signIn("hamqth", " W1AW ", "hpw").isEmpty() && settings.hamqthUser() == "W1AW" &&
                  settings.lookupService() == "hamqth",
              "hamqth signed in");
        check(settings.signIn("qrz", "W1AW", "qpw").isEmpty() && settings.lookupService() == "qrz" &&
                  settings.lookupPassword() == "qpw" && settings.accountSignedIn("hamqth"),
              "both signed in: lookups use QRZ.com");
        check(settings.signIn("qrz", "w1aw", "").isEmpty() && settings.lookupPassword() == "qpw",
              "empty password keeps the saved one for the same user");
        check(!settings.signIn("qrz", "K1ABC", "").isEmpty(), "a different user needs its password");
        check(settings.signOut("qrz").isEmpty() && settings.lookupService() == "hamqth" && settings.qrzUser().isEmpty(),
              "signing out of QRZ.com falls back to HamQTH");
        check(!settings.signIn("hamalert", "W1AW", "has space").isEmpty(), "hamalert password with a space refused");
        check(settings.signIn("hamalert", "W1AW", "apw!").isEmpty() && settings.accountUser("hamalert") == "W1AW" &&
                  settings.accountSignedIn("hamalert"),
              "hamalert signed in");
        check(!settings.signIn("hamsat", {}, "").isEmpty(), "hams.at needs a key");
        check(settings.signIn("hamsat", {}, " key-123 ").isEmpty() && settings.hamsatKey() == "key-123" &&
                  settings.accountSignedIn("hamsat"),
              "hams.at key saved");
        check(settings.signOut("hamalert").isEmpty() && !settings.accountSignedIn("hamalert") && settings.hamAlertUser().isEmpty(),
              "hamalert signed out");

        // Nothing secret outside credentials.ini: not options.ini, not a saved configuration.
        check(config_save(dir.path(), "Field day").isEmpty(), "configuration saved");
        const auto options = file_text(settings.optionsPath());
        const auto snapshot = file_text(config_path(dir.path(), "Field day"));
        for (const char* secret : {"hpw", "qpw", "key-123", "apw!"})
            check(!options.contains(secret) && !snapshot.contains(secret), secret);
        AppSettings reloaded({}, dir.path());
        check(reloaded.hamqthUser() == "W1AW" && reloaded.lookupService() == "hamqth" && reloaded.hamsatKey() == "key-123" &&
                  !reloaded.accountSignedIn("qrz") && !reloaded.accountSignedIn("hamalert"),
              "sign-ins survive a restart");
    }

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
