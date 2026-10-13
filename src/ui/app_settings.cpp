#include "app_settings.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <exception>

namespace hamclock {
namespace {
constexpr int kMaxHostLength = 253;
constexpr int kMaxUserLength = 64;
constexpr int kMaxPasswordLength = 128;

QString default_config_dir() {
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir.isEmpty() ? QDir::homePath() + "/.config/hamclock-pro" : dir;
}
} // namespace

AppSettings::AppSettings(QString station_path, QString config_dir, QObject* parent)
    : QObject(parent), config_dir_(config_dir.isEmpty() ? default_config_dir() : std::move(config_dir)),
      station_path_(station_path.isEmpty() ? config_dir_ + "/station.conf" : std::move(station_path)) {
    QDir().mkpath(config_dir_);
    if (QFileInfo::exists(station_path_)) {
        try {
            station_ = load_settings(station_path_.toStdString());
            configured_ = true;
        } catch (const std::exception& error) {
            load_error_ = QString("Station file could not be read: %1").arg(error.what());
        }
    }
    loadOptions();
    loadAccounts();
}

void AppSettings::setAppearance(const QString& appearance) {
    if (appearance != "system" && appearance != "light" && appearance != "dark") return;
    if (appearance.toStdString() == station_.appearance) return;
    station_.appearance = appearance.toStdString();
    if (configured_) {
        try { save_settings(station_path_.toStdString(), station_); } catch (const std::exception&) {}
    }
    emit appearanceChanged();
}

QString AppSettings::saveStation(const QString& callsign, double de_lat, double de_lon,
                                 double dx_lat, double dx_lon, const QString& de_zone) {
    auto next = station_;
    next.callsign = callsign.trimmed().toUpper().toStdString();
    next.de = {de_lat, de_lon};
    next.dx = {dx_lat, dx_lon};
    next.de_zone = de_zone.trimmed().toStdString();
    try {
        save_settings(station_path_.toStdString(), next);
    } catch (const std::exception& error) {
        return QString::fromUtf8(error.what());
    }
    station_ = std::move(next);
    configured_ = true;
    load_error_.clear();
    emit stationChanged();
    return {};
}

void AppSettings::loadOptions() {
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    cluster_host_ = ini.value("cluster/host").toString().left(kMaxHostLength);
    const int port = ini.value("cluster/port", 7300).toInt();
    cluster_port_ = port > 0 && port < 65536 ? port : 7300;
}

QString AppSettings::saveCluster(const QString& host, int port) {
    static const QRegularExpression valid_host("^[A-Za-z0-9.-]*$");
    const auto trimmed = host.trimmed();
    if (trimmed.size() > kMaxHostLength || !valid_host.match(trimmed).hasMatch())
        return "Cluster host may contain only letters, digits, dots and hyphens.";
    if (port <= 0 || port >= 65536) return "Cluster port must be 1–65535.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("cluster/host", trimmed);
    ini.setValue("cluster/port", port);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save cluster settings.";
    cluster_host_ = trimmed;
    cluster_port_ = port;
    emit optionsChanged();
    return {};
}

void AppSettings::loadAccounts() {
    QSettings ini(config_dir_ + "/credentials.ini", QSettings::IniFormat);
    const auto service = ini.value("lookup/service", "none").toString();
    lookup_service_ = service == "qrz" || service == "hamqth" ? service : "none";
    lookup_user_ = ini.value("lookup/user").toString().left(kMaxUserLength);
    lookup_password_ = ini.value("lookup/password").toString().left(kMaxPasswordLength);
}

QString AppSettings::writeAccounts() {
    const auto path = config_dir_ + "/credentials.ini";
    {
        QSettings ini(path, QSettings::IniFormat);
        ini.clear();
        ini.setValue("lookup/service", lookup_service_);
        ini.setValue("lookup/user", lookup_user_);
        ini.setValue("lookup/password", lookup_password_);
        ini.sync();
        if (ini.status() != QSettings::NoError) return "Could not save account settings.";
    }
    // Owner read/write only; credentials must not be readable by other users.
    if (!QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner))
        return "Saved, but could not restrict credentials file permissions.";
    return {};
}

QString AppSettings::saveAccount(const QString& service, const QString& user, const QString& password) {
    if (service != "none" && service != "qrz" && service != "hamqth") return "Unknown lookup service.";
    const auto trimmed_user = user.trimmed();
    if (trimmed_user.size() > kMaxUserLength || password.size() > kMaxPasswordLength)
        return "User name or password is too long.";
    if (service != "none" && trimmed_user.isEmpty()) return "Enter a user name for the lookup service.";
    lookup_service_ = service;
    lookup_user_ = service == "none" ? QString() : trimmed_user;
    if (service == "none") lookup_password_.clear();
    else if (!password.isEmpty()) lookup_password_ = password;
    const auto error = writeAccounts();
    emit accountsChanged();
    return error;
}

void AppSettings::clearAccount() { saveAccount("none", {}, {}); }

} // namespace hamclock
