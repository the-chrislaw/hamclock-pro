#include "credential_store.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace hamclock {

namespace {
QJsonObject readAll(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(f.readAll()).object();
}

bool writeAll(const QString& path, const QJsonObject& obj) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    return f.commit();
}
} // namespace

FileCredentialStore::FileCredentialStore(QString path) : m_path(std::move(path)) {}

bool FileCredentialStore::set(const QString& key, const QString& value) {
    QJsonObject obj = readAll(m_path);
    obj.insert(key, value);
    return writeAll(m_path, obj);
}

QString FileCredentialStore::get(const QString& key) const {
    return readAll(m_path).value(key).toString();
}

bool FileCredentialStore::remove(const QString& key) {
    QJsonObject obj = readAll(m_path);
    if (!obj.contains(key)) {
        return true;
    }
    obj.remove(key);
    return writeAll(m_path, obj);
}

#ifndef Q_OS_DARWIN
std::unique_ptr<CredentialStore> makeDefaultCredentialStore() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return std::make_unique<FileCredentialStore>(dir + QStringLiteral("/credentials.json"));
}
#endif

} // namespace hamclock
