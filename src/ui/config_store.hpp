#pragma once
// Saved configurations (A16): named copies of options.ini in <config>/configs/<name>.ini.
// Station (station.ini) and credentials are never part of a snapshot.
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace hamclock {

constexpr int kMaxConfigs = 12;
constexpr int kConfigNameMax = 24;

// Letters, digits, space, '-' and '_' only; trimmed, 1..24 characters. "" = invalid.
inline QString config_clean_name(const QString& raw) {
    const QString n = raw.simplified();
    static const QRegularExpression ok(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9 _-]*$"));
    return n.size() <= kConfigNameMax && ok.match(n).hasMatch() ? n : QString();
}

inline QString config_dir(const QString& root) { return root + QStringLiteral("/configs"); }
inline QString config_path(const QString& root, const QString& name) {
    return config_dir(root) + QLatin1Char('/') + name + QStringLiteral(".ini");
}

inline QStringList config_list(const QString& root) {
    QStringList out;
    for (const QString& f : QDir(config_dir(root)).entryList({QStringLiteral("*.ini")}, QDir::Files, QDir::IgnoreCase | QDir::Name))
        out << f.left(f.size() - 4);
    return out;
}

inline bool config_exists(const QString& root, const QString& name) { return QFile::exists(config_path(root, name)); }

// Returns "" on success, else a message for the user.
inline QString config_save(const QString& root, const QString& rawName) {
    const QString name = config_clean_name(rawName);
    if (name.isEmpty()) return QStringLiteral("Use 1-24 letters, digits, spaces, - or _.");
    if (!config_exists(root, name) && config_list(root).size() >= kMaxConfigs) return QStringLiteral("At most 12 saved configurations.");
    if (!QDir().mkpath(config_dir(root))) return QStringLiteral("Could not create the configurations folder.");
    const QString dest = config_path(root, name), src = root + QStringLiteral("/options.ini");
    QFile::remove(dest);
    if (QFile::exists(src)) return QFile::copy(src, dest) ? QString() : QStringLiteral("Could not save the configuration.");
    QFile empty(dest);  // nothing customised yet: an empty snapshot restores defaults
    return empty.open(QIODevice::WriteOnly) ? QString() : QStringLiteral("Could not save the configuration.");
}

inline QString config_restore(const QString& root, const QString& rawName) {
    const QString name = config_clean_name(rawName);
    if (name.isEmpty() || !config_exists(root, name)) return QStringLiteral("No such configuration.");
    const QString dest = root + QStringLiteral("/options.ini"), tmp = dest + QStringLiteral(".new");
    QFile::remove(tmp);
    if (!QFile::copy(config_path(root, name), tmp)) return QStringLiteral("Could not restore the configuration.");
    QFile::remove(dest);
    return QFile::rename(tmp, dest) ? QString() : QStringLiteral("Could not restore the configuration.");
}

inline QString config_delete(const QString& root, const QString& rawName) {
    const QString name = config_clean_name(rawName);
    if (name.isEmpty() || !config_exists(root, name)) return QStringLiteral("No such configuration.");
    return QFile::remove(config_path(root, name)) ? QString() : QStringLiteral("Could not delete the configuration.");
}

inline QString config_rename(const QString& root, const QString& rawFrom, const QString& rawTo) {
    const QString from = config_clean_name(rawFrom), to = config_clean_name(rawTo);
    if (to.isEmpty()) return QStringLiteral("Use 1-24 letters, digits, spaces, - or _.");
    if (from.isEmpty() || !config_exists(root, from)) return QStringLiteral("No such configuration.");
    if (from == to) return {};
    if (config_exists(root, to) && from.compare(to, Qt::CaseInsensitive) != 0) return QStringLiteral("That name is taken.");
    return QFile::rename(config_path(root, from), config_path(root, to)) ? QString() : QStringLiteral("Could not rename the configuration.");
}

}  // namespace hamclock
