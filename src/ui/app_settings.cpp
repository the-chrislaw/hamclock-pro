#include <algorithm>
#include "app_settings.hpp"
#include "../net/telnet_client.hpp"
#include "../widgets/watchlist/watch_match.hpp"
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
    const int ssid = ini.value("cluster/ssid", 0).toInt();
    cluster_ssid_ = ssid >= 0 && ssid <= 99 ? ssid : 0;
    cluster_commands_ = TelnetClient::sanitize_commands(ini.value("cluster/commands").toStringList());
    watch_list_ = watchlist::sanitize(ini.value("watch/list").toStringList());
    adif_path_ = ini.value("adif/path").toString().left(1024);
    ota_program_ = ini.value("ota/program").toString();
    if (ota_program_ != "POTA" && ota_program_ != "SOTA") ota_program_.clear();
    static const QRegularExpression band_re("^[0-9]{1,4}(\\.[0-9]+)?(m|cm|mm)$");
    static const QRegularExpression mode_re("^[A-Za-z0-9-]{1,10}$");
    ota_band_ = ini.value("ota/band").toString();
    if (!band_re.match(ota_band_).hasMatch()) ota_band_.clear();
    ota_mode_ = ini.value("ota/mode").toString();
    if (!mode_re.match(ota_mode_).hasMatch()) ota_mode_.clear();
    const int age = ini.value("ota/max_age_min", 0).toInt();
    ota_max_age_min_ = age > 0 && age <= 1440 ? age : 0;
    ota_sort_ = ini.value("ota/sort").toString();
    if (ota_sort_ != "freq" && ota_sort_ != "dist") ota_sort_.clear();
    static const QRegularExpression aprs_host_re("^[A-Za-z0-9.-]{1,253}$");
    const QString ah = ini.value("aprs/host").toString();
    if (aprs_host_re.match(ah).hasMatch()) aprs_host_ = ah;
    const int ap = ini.value("aprs/port", 14580).toInt();
    aprs_port_ = ap > 0 && ap < 65536 ? ap : 14580;
    const int ar = ini.value("aprs/radius_km", 100).toInt();
    aprs_radius_km_ = ar >= 1 && ar <= 500 ? ar : 100;
    const int udp = ini.value("udp/port", 0).toInt();
    udp_port_ = udp >= 1024 && udp < 65536 ? udp : 0;
    udp_lan_ = ini.value("udp/lan", false).toBool();
    imperial_ = ini.value("units/imperial", false).toBool();
    date_order_ = ini.value("clock/date_order", 0).toInt();
    if (date_order_ < 0 || date_order_ > 2) date_order_ = 0;
    clock_12h_ = ini.value("clock/h12", false).toBool();
    week_sunday_ = ini.value("clock/week_start", 1).toInt() == 0;
    static const QRegularExpression valid_id("^[a-z][a-z0-9_]*$");
    panes_ = QStringList();
    for (int slot = 0; slot < kPaneSlots; ++slot) {
        const auto id = ini.value(QString("panes/slot%1").arg(slot)).toString();
        panes_ << (id == "none" || valid_id.match(id).hasMatch() ? id : QString());
    }
    rotations_.clear();
    for (int slot = 0; slot < kPaneSlots; ++slot) {
        const int seconds = ini.value(QString("panes/rotate%1").arg(slot), 0).toInt();
        rotations_ << (validRotation(seconds) ? seconds : 0);
    }
    rotation_sets_.clear();
    for (int slot = 0; slot < kPaneSlots; ++slot) {
        QStringList ids;
        for (const auto& id : ini.value(QString("panes/set%1").arg(slot)).toStringList())
            if (valid_id.match(id).hasMatch() && id != "none" && !ids.contains(id) && ids.size() < kMaxRotationSet) ids << id;
        rotation_sets_ << ids;
    }
}

QStringList AppSettings::panes() const {
    static const QStringList defaults{"spaceweather", "solarwind", "dxcluster"};
    QStringList result;
    for (int slot = 0; slot < kPaneSlots; ++slot)
        result << (slot < panes_.size() && !panes_[slot].isEmpty() ? panes_[slot] : defaults[slot]);
    return result;
}

QList<int> AppSettings::paneRotations() const {
    QList<int> result = rotations_;
    while (result.size() < kPaneSlots) result << 0;
    return result;
}

QString AppSettings::setPaneRotation(int slot, int seconds) {
    if (slot < 0 || slot >= kPaneSlots) return "No such pane.";
    if (!validRotation(seconds)) return "Rotation must be off, 5, 10, 30 or 60 seconds.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue(QString("panes/rotate%1").arg(slot), seconds);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the rotation.";
    rotations_ = paneRotations();
    rotations_[slot] = seconds;
    emit panesChanged();
    return {};
}

QVariantList AppSettings::paneRotationSets() const {
    QVariantList result;
    for (int slot = 0; slot < kPaneSlots; ++slot)
        result << (slot < rotation_sets_.size() ? rotation_sets_[slot] : QStringList());
    return result;
}

QString AppSettings::togglePaneRotationMember(int slot, const QString& widget_id) {
    static const QRegularExpression valid_id("^[a-z][a-z0-9_]*$");
    if (slot < 0 || slot >= kPaneSlots) return "No such pane.";
    if (!valid_id.match(widget_id).hasMatch() || widget_id == "none") return "Unknown widget.";
    while (rotation_sets_.size() < kPaneSlots) rotation_sets_ << QStringList();
    auto ids = rotation_sets_[slot];
    if (!ids.removeOne(widget_id)) {
        if (ids.size() >= kMaxRotationSet) return "The rotation set is full.";
        ids << widget_id;
    }
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue(QString("panes/set%1").arg(slot), ids);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the rotation set.";
    rotation_sets_[slot] = ids;
    emit panesChanged();
    return {};
}

QString AppSettings::clearPaneRotationSet(int slot) {
    if (slot < 0 || slot >= kPaneSlots) return "No such pane.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.remove(QString("panes/set%1").arg(slot));
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the rotation set.";
    while (rotation_sets_.size() < kPaneSlots) rotation_sets_ << QStringList();
    rotation_sets_[slot].clear();
    emit panesChanged();
    return {};
}

QString AppSettings::setPane(int slot, const QString& widget_id) {
    static const QRegularExpression valid_id("^[a-z][a-z0-9_]*$");
    if (slot < 0 || slot >= kPaneSlots) return "No such pane.";
    if (!valid_id.match(widget_id).hasMatch()) return "Unknown widget.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue(QString("panes/slot%1").arg(slot), widget_id);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the pane choice.";
    while (panes_.size() < kPaneSlots) panes_ << QString();
    panes_[slot] = widget_id;
    emit panesChanged();
    return {};
}

void AppSettings::resetPanes() {
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.remove("panes");
    ini.sync();
    panes_.clear();
    rotations_.clear();
    rotation_sets_.clear();
    emit panesChanged();
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

QString AppSettings::saveClusterLogin(int ssid, const QString& commands) {
    if (ssid < 0 || ssid > 99) return "SSID must be 0 (none) to 99.";
    const auto wanted = commands.split('\n', Qt::SkipEmptyParts);
    const auto kept = TelnetClient::sanitize_commands(wanted);
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("cluster/ssid", ssid);
    ini.setValue("cluster/commands", kept);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save cluster login.";
    cluster_ssid_ = ssid;
    cluster_commands_ = kept;
    emit optionsChanged();
    if (kept.size() < wanted.size()) return QString("Saved; %1 command(s) skipped (only short receive-only commands are allowed).").arg(wanted.size() - kept.size());
    return {};
}

void AppSettings::loadAccounts() {
    QSettings ini(config_dir_ + "/credentials.ini", QSettings::IniFormat);
    const auto service = ini.value("lookup/service", "none").toString();
    lookup_service_ = service == "qrz" || service == "hamqth" ? service : "none";
    lookup_user_ = ini.value("lookup/user").toString().left(kMaxUserLength);
    lookup_password_ = ini.value("lookup/password").toString().left(kMaxPasswordLength);
    hamalert_user_ = ini.value("hamalert/user").toString().left(kMaxUserLength);
    hamalert_password_ = ini.value("hamalert/password").toString().left(kMaxPasswordLength);
}

QString AppSettings::writeAccounts() {
    const auto path = config_dir_ + "/credentials.ini";
    {
        QSettings ini(path, QSettings::IniFormat);
        ini.clear();
        ini.setValue("lookup/service", lookup_service_);
        ini.setValue("lookup/user", lookup_user_);
        ini.setValue("lookup/password", lookup_password_);
        ini.setValue("hamalert/user", hamalert_user_);
        ini.setValue("hamalert/password", hamalert_password_);
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

QString AppSettings::saveHamAlert(const QString& user, const QString& password) {
    const auto trimmed_user = user.trimmed();
    if (trimmed_user.size() > kMaxUserLength || password.size() > kMaxPasswordLength)
        return "User name or password is too long.";
    const auto printable = [](const QString& v) {
        return std::all_of(v.begin(), v.end(), [](QChar c) { return c.unicode() > 32 && c.unicode() < 127; });
    };
    if (!printable(trimmed_user) || !printable(password)) return "Use plain letters, digits and symbols only (no spaces).";
    hamalert_user_ = trimmed_user;
    if (trimmed_user.isEmpty()) hamalert_password_.clear();
    else if (!password.isEmpty()) hamalert_password_ = password;
    const auto error = writeAccounts();
    emit accountsChanged();
    return error;
}

void AppSettings::clearAccount() { saveAccount("none", {}, {}); }

QString AppSettings::saveWatchList(const QString& entries) {
    const auto wanted = entries.split(QRegularExpression("[\\s,;]+"), Qt::SkipEmptyParts);
    const auto kept = watchlist::sanitize(wanted);
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("watch/list", kept);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the watch list.";
    watch_list_ = kept;
    emit optionsChanged();
    if (kept.size() < wanted.size()) return QString("Saved; %1 entry(ies) skipped (letters, digits, / * ? only; duplicates removed).").arg(wanted.size() - kept.size());
    return {};
}

QString AppSettings::saveAdifPath(const QString& path) {
    const QString kept = path.trimmed().left(1024);
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("adif/path", kept);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the log path.";
    adif_path_ = kept;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveOtaSort(const QString& sort) {
    const auto s = sort.trimmed().toLower();
    if (!s.isEmpty() && s != "time" && s != "freq" && s != "dist") return "Sort must be time, freq or dist.";
    const QString kept = s == "time" ? QString() : s;
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("ota/sort", kept);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the sort order.";
    ota_sort_ = kept;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveOtaFilter(const QString& program, const QString& band, const QString& mode, int max_age_min) {
    const auto prog = program.trimmed().toUpper();
    const auto b = band.trimmed().toLower();
    const auto m = mode.trimmed().toUpper();
    static const QRegularExpression band_re("^[0-9]{1,4}(\\.[0-9]+)?(m|cm|mm)$");
    static const QRegularExpression mode_re("^[A-Z0-9-]{1,10}$");
    if (!prog.isEmpty() && prog != "POTA" && prog != "SOTA") return "Program must be POTA, SOTA or empty.";
    if (!b.isEmpty() && !band_re.match(b).hasMatch()) return "Band looks like 20m or 70cm.";
    if (!m.isEmpty() && !mode_re.match(m).hasMatch()) return "Mode: letters and digits only (CW, SSB, FT8).";
    if (max_age_min < 0 || max_age_min > 1440) return "Maximum age is 0-1440 minutes (0 = any).";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("ota/program", prog);
    ini.setValue("ota/band", b);
    ini.setValue("ota/mode", m);
    ini.setValue("ota/max_age_min", max_age_min);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the filter.";
    ota_program_ = prog;
    ota_band_ = b;
    ota_mode_ = m;
    ota_max_age_min_ = max_age_min;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveAprs(const QString& host, int port, int radius_km) {
    static const QRegularExpression host_re("^[A-Za-z0-9.-]{1,253}$");
    QString h = host.trimmed();
    if (h.isEmpty()) h = "rotate.aprs2.net";
    if (!host_re.match(h).hasMatch()) return "APRS server may contain only letters, digits, dots and hyphens.";
    if (port <= 0 || port >= 65536) return "APRS port must be 1-65535.";
    if (radius_km < 1 || radius_km > 500) return "APRS radius is 1-500 km.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("aprs/host", h);
    ini.setValue("aprs/port", port);
    ini.setValue("aprs/radius_km", radius_km);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the APRS settings.";
    const bool changed = h != aprs_host_ || port != aprs_port_ || radius_km != aprs_radius_km_;
    aprs_host_ = h;
    aprs_port_ = port;
    aprs_radius_km_ = radius_km;
    emit optionsChanged();
    if (changed) emit aprsChanged();
    return {};
}

QString AppSettings::saveImperialUnits(bool imperial) {
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("units/imperial", imperial);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the units.";
    imperial_ = imperial;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveClockFormat(int date_order, bool h12) {
    if (date_order < 0 || date_order > 2) return "Unknown date order.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("clock/date_order", date_order);
    ini.setValue("clock/h12", h12);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the clock format.";
    date_order_ = date_order;
    clock_12h_ = h12;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveUdpSpots(int port, bool lan) {
    if (port != 0 && (port < 1024 || port > 65535)) return "UDP port must be 0 (off) or 1024-65535.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("udp/port", port);
    ini.setValue("udp/lan", lan);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the UDP spot settings.";
    udp_port_ = port;
    udp_lan_ = lan;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveWeekStart(bool sunday) {
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("clock/week_start", sunday ? 0 : 1);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the week start.";
    week_sunday_ = sunday;
    emit optionsChanged();
    return {};
}

} // namespace hamclock
