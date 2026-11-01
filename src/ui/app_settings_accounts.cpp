// AppSettings: account sign-ins for callsign lookup (QRZ.com, HamQTH), HamAlert and hams.at.
// User names live in <config>/credentials.ini (owner-only, 0600). Passwords and keys live in
// the secret store (the Keychain on Apple devices) when there is one, otherwise in that same
// file. Nothing here ever reaches options.ini, saved configurations, logs or status text.
#include "app_settings.hpp"
#include "../platform/credential_store.hpp"
#include "../widgets/dxcluster/cluster_nodes.hpp"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <algorithm>
#include <array>

namespace hamclock {
namespace {
constexpr int kMaxUserLength = 64;
constexpr int kMaxSecretLength = 128;

struct SecretSlot {
    const char* service;
    const char* ini_key;    // credentials.ini, used without a secret store
    const char* store_key;  // secret store entry (suffixed per settings directory)
};
constexpr std::array<SecretSlot, 4> kSecrets{{
    {"qrz", "qrz/password", "qrz.password"},
    {"hamqth", "hamqth/password", "hamqth.password"},
    {"hamalert", "hamalert/password", "hamalert.password"},
    {"hamsat", "hamsat/key", "hamsat.key"},
}};
// Services with a user name (hams.at has a key only).
const QStringList kUserServices{"qrz", "hamqth", "hamalert"};
const QStringList kServices{"qrz", "hamqth", "hamalert", "hamsat"};
// Before 1.0 a single lookup account was kept, its service named in lookup/service.
constexpr SecretSlot kLegacyLookup{"", "lookup/password", "lookup.password"};

// Printable ASCII without spaces: what a telnet login line and an API key can carry.
bool plain(const QString& v) {
    return std::all_of(v.begin(), v.end(), [](QChar c) { return c.unicode() > 32 && c.unicode() < 127; });
}
} // namespace

// One set of secrets per settings directory, so a scratch profile (-c DIR) never
// reads, overwrites or deletes the passwords of the real one.
QString AppSettings::secretKey(const QString& store_key) const {
    const auto dir = QFileInfo(config_dir_).absoluteFilePath().toUtf8();
    return store_key + "@" + QCryptographicHash::hash(dir, QCryptographicHash::Sha1).toHex().left(16);
}

void AppSettings::setSecretStore(std::unique_ptr<CredentialStore> store) {
    secrets_ = std::move(store);
    if (!secrets_) return;
    // Move secrets saved by an earlier version out of the file. A value only leaves
    // the file once the store holds one; if the store refuses, it stays where it is.
    QSettings ini(config_dir_ + "/credentials.ini", QSettings::IniFormat);
    bool moved = false;
    for (const auto& slot : kSecrets) {
        const auto value = ini.value(slot.ini_key).toString();
        if (value.isEmpty()) continue;
        const auto key = secretKey(slot.store_key);
        if (secrets_->get(key).isEmpty() && !secrets_->set(key, value)) continue;
        ini.remove(slot.ini_key);
        moved = true;
    }
    if (moved) ini.sync();
    loadAccounts();
    emit accountsChanged();
}

void AppSettings::loadAccounts() {
    QSettings ini(config_dir_ + "/credentials.ini", QSettings::IniFormat);
    // From the store when there is one; the file is the fallback (the Pi, or a value
    // the store refused to take during migration).
    const auto secret = [&](const SecretSlot& slot) {
        QString value = secrets_ ? secrets_->get(secretKey(slot.store_key)) : QString();
        if (value.isEmpty()) value = ini.value(slot.ini_key).toString();
        return value.left(kMaxSecretLength);
    };
    account_user_.clear();
    account_secret_.clear();
    for (const auto& service : kUserServices) {
        const auto user = ini.value(service + "/user").toString().left(kMaxUserLength);
        if (!user.isEmpty()) account_user_.insert(service, user);
    }
    for (const auto& slot : kSecrets)
        if (const auto value = secret(slot); !value.isEmpty()) account_secret_.insert(slot.service, value);

    // Single lookup account from before 1.0 -> its own service's slot. It is written back
    // (and the old entries removed) only once its password is found: on Apple devices that
    // is after setSecretStore(), so the constructor's first read never drops the Keychain link.
    const auto old_service = ini.value("lookup/service").toString();
    const auto old_user = ini.value("lookup/user").toString().left(kMaxUserLength);
    if ((old_service != "qrz" && old_service != "hamqth") || old_user.isEmpty() || account_user_.contains(old_service))
        return;
    account_user_.insert(old_service, old_user);
    const auto old_password = secret(kLegacyLookup);
    if (old_password.isEmpty()) return;
    account_secret_.insert(old_service, old_password);
    if (writeAccounts().isEmpty() && secrets_) secrets_->remove(secretKey(kLegacyLookup.store_key));
}

QString AppSettings::writeAccounts() {
    const auto path = config_dir_ + "/credentials.ini";
    // Store first: if it refuses, the file (which may still hold a fallback copy) is left alone.
    if (secrets_) {
        for (const auto& slot : kSecrets) {
            const auto value = account_secret_.value(slot.service);
            const auto key = secretKey(slot.store_key);
            if (!(value.isEmpty() ? secrets_->remove(key) : secrets_->set(key, value)))
                return "Could not save the password in the Keychain.";
        }
    }
    {
        QSettings ini(path, QSettings::IniFormat);
        ini.clear();  // also drops the pre-1.0 lookup/* entries
        for (const auto& service : kUserServices)
            if (account_user_.contains(service)) ini.setValue(service + "/user", account_user_.value(service));
        if (!secrets_)
            for (const auto& slot : kSecrets)
                if (account_secret_.contains(slot.service)) ini.setValue(slot.ini_key, account_secret_.value(slot.service));
        ini.sync();
        if (ini.status() != QSettings::NoError) return "Could not save account settings.";
    }
    // Nothing left to keep: QSettings writes no file, so there is nothing to protect.
    if (!QFile::exists(path)) return {};
    // Owner read/write only; credentials must not be readable by other users.
    if (!QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner))
        return "Saved, but could not restrict credentials file permissions.";
    return {};
}

QString AppSettings::lookupService() const {
    for (const char* service : {"qrz", "hamqth"})  // QRZ.com first when both are signed in
        if (accountSignedIn(service)) return service;
    return "none";
}

bool AppSettings::accountSignedIn(const QString& service) const {
    if (service == "hamsat") return account_secret_.contains("hamsat");
    return account_user_.contains(service) && account_secret_.contains(service);
}

// Sets one service's login (empty user or secret removes it), saves and announces it.
QString AppSettings::storeAccount(const QString& service, const QString& user, const QString& secret) {
    if (user.isEmpty()) account_user_.remove(service);
    else account_user_.insert(service, user);
    if (secret.isEmpty()) account_secret_.remove(service);
    else account_secret_.insert(service, secret);
    const auto error = writeAccounts();
    emit accountsChanged();
    return error;
}

QString AppSettings::signIn(const QString& service, const QString& user, const QString& secret) {
    if (!kServices.contains(service)) return "Unknown account.";
    if (service == "hamsat") {
        const auto key = secret.trimmed();
        if (key.isEmpty()) return hasHamsatKey() ? QString() : "Paste the API key from your hams.at Settings page.";
        if (key.size() > kMaxSecretLength) return "The key is too long.";
        if (!plain(key)) return "Use plain letters, digits and symbols only (no spaces).";
        return storeAccount(service, {}, key);
    }
    const auto name = user.trimmed();
    if (name.isEmpty()) return "Enter the user name.";
    if (name.size() > kMaxUserLength || secret.size() > kMaxSecretLength) return "User name or password is too long.";
    if (service == "hamalert" && (!plain(name) || !plain(secret)))
        return "Use plain letters, digits and symbols only (no spaces).";
    // An empty password keeps the saved one, but only for the same user.
    const bool same_user = name.compare(account_user_.value(service), Qt::CaseInsensitive) == 0;
    const auto kept = secret.isEmpty() && same_user ? account_secret_.value(service) : secret;
    if (kept.isEmpty()) return "Enter the password.";
    return storeAccount(service, name, kept);
}

QString AppSettings::signOut(const QString& service) {
    if (!kServices.contains(service)) return "Unknown account.";
    return storeAccount(service, {}, {});
}

QString AppSettings::saveAccount(const QString& service, const QString& user, const QString& password) {
    if (service != "none" && service != "qrz" && service != "hamqth") return "Unknown lookup service.";
    const auto trimmed_user = user.trimmed();
    if (trimmed_user.size() > kMaxUserLength || password.size() > kMaxSecretLength)
        return "User name or password is too long.";
    if (service != "none" && trimmed_user.isEmpty()) return "Enter a user name for the lookup service.";
    for (const char* other : {"qrz", "hamqth"})
        if (other != service) { account_user_.remove(other); account_secret_.remove(other); }
    if (service == "none") return storeAccount("qrz", {}, {});
    const auto kept = password.isEmpty() ? account_secret_.value(service) : password;
    return storeAccount(service, trimmed_user, kept);
}

QString AppSettings::saveHamAlert(const QString& user, const QString& password) {
    const auto trimmed_user = user.trimmed();
    if (trimmed_user.size() > kMaxUserLength || password.size() > kMaxSecretLength)
        return "User name or password is too long.";
    if (!plain(trimmed_user) || !plain(password)) return "Use plain letters, digits and symbols only (no spaces).";
    if (trimmed_user.isEmpty()) return storeAccount("hamalert", {}, {});
    const auto kept = password.isEmpty() ? hamAlertPassword() : password;
    return storeAccount("hamalert", trimmed_user, kept);
}

QString AppSettings::saveHamsatKey(const QString& key) {
    const auto trimmed = key.trimmed();
    if (trimmed.size() > kMaxSecretLength) return "The key is too long.";
    if (!plain(trimmed)) return "Use plain letters, digits and symbols only (no spaces).";
    return storeAccount("hamsat", {}, trimmed);
}

void AppSettings::clearAccount() { saveAccount("none", {}, {}); }

QVariantList AppSettings::clusterSuggestions() const {
    QVariantList out;
    for (const auto& node : dxcluster::kSuggestedNodes)
        out.push_back(QVariantMap{{"label", node.label}, {"host", node.host}, {"port", node.port}, {"note", node.note}});
    return out;
}

} // namespace hamclock
