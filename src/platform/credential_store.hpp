#pragma once

#include <QString>
#include <memory>

namespace hamclock {

// Secret storage for passwords and API keys (HamAlert, hams.at, lookup
// services). Values are never logged. Keys are plain identifiers such as
// "hamalert.password".
class CredentialStore {
public:
    virtual ~CredentialStore() = default;
    // Returns false if the value could not be stored.
    virtual bool set(const QString& key, const QString& value) = 0;
    // Returns an empty string when the key is absent.
    virtual QString get(const QString& key) const = 0;
    // Removing an absent key succeeds.
    virtual bool remove(const QString& key) = 0;
};

// Owner-only (0600) JSON file in a 0700 directory; used on Linux/Pi and as the
// portable fallback. Values are stored in clear text (no encryption at rest).
class FileCredentialStore final : public CredentialStore {
public:
    explicit FileCredentialStore(QString path);
    bool set(const QString& key, const QString& value) override;
    QString get(const QString& key) const override;
    bool remove(const QString& key) override;

private:
    QString m_path;
};

#ifdef Q_OS_DARWIN
// macOS / iOS Keychain (generic passwords under a service name).
class KeychainCredentialStore final : public CredentialStore {
public:
    explicit KeychainCredentialStore(QString service);
    bool set(const QString& key, const QString& value) override;
    QString get(const QString& key) const override;
    bool remove(const QString& key) override;
    // False when no usable keychain exists (e.g. locked or headless runner).
    bool available() const;

private:
    QString m_service;
};
#endif

// Keychain on Apple platforms, otherwise a file under the app data location.
std::unique_ptr<CredentialStore> makeDefaultCredentialStore();

} // namespace hamclock
