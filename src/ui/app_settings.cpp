#include <algorithm>
#include "app_settings.hpp"
#include "banner_text.hpp"
#include "blanking.hpp"
#include "config_store.hpp"
#include "../net/telnet_client.hpp"
#include "../widgets/watchlist/watch_match.hpp"
#include "../widgets/pskreporter/psk_overlay.hpp"
#include <QDir>
#include <QHostAddress>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QTimeZone>
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
        try { save_settings(station_path_.toStdString(), persistable()); } catch (const std::exception&) {}
    }
    emit appearanceChanged();
}

QString AppSettings::saveStation(const QString& callsign, double de_lat, double de_lon,
                                 double dx_lat, double dx_lon, const QString& de_zone) {
    auto next = station_;
    // Set-DE passes the displayed (override) DX back; only a different value is an explicit DX edit.
    const bool kept_override = dx_override_ && dx_lat == station_.dx.latitude && dx_lon == station_.dx.longitude;
    next.callsign = callsign.trimmed().toUpper().toStdString();
    next.de = {de_lat, de_lon};
    next.dx = kept_override ? saved_dx_ : hamclock::Location{dx_lat, dx_lon};
    next.de_zone = de_zone.trimmed().toStdString();
    try {
        save_settings(station_path_.toStdString(), next);
    } catch (const std::exception& error) {
        return QString::fromUtf8(error.what());
    }
    if (kept_override) {
        saved_dx_ = next.dx;
        next.dx = station_.dx;
    } else {
        dx_override_ = false;
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
    if (ota_program_ != "POTA" && ota_program_ != "SOTA" && ota_program_ != "WWFF") ota_program_.clear();
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
    const QString mh = ini.value("mesh/host").toString().trimmed();
    if (aprs_host_re.match(mh).hasMatch()) mesh_host_ = mh;
    const int mp = ini.value("mesh/port", 4403).toInt();
    mesh_port_ = mp > 0 && mp < 65536 ? mp : 4403;
    const int pk = ini.value("psk/max_km", 0).toInt();
    psk_max_km_ = pk >= 0 && pk <= 20000 ? pk : 0;
    psk_match_ = ini.value("psk/match").toString().left(120);
    psk_wspr_ = ini.value("psk/wspr", false).toBool();
    psk_rbn_ = ini.value("psk/rbn", false).toBool();
    const int udp = ini.value("udp/port", 0).toInt();
    udp_port_ = udp >= 1024 && udp < 65536 ? udp : 0;
    udp_lan_ = ini.value("udp/lan", false).toBool();
    {
        const QString g = ini.value("udp/group").toString().trimmed();
        QHostAddress a;
        udp_group_ = a.setAddress(g) && a.protocol() == QAbstractSocket::IPv4Protocol && a.isMulticast() ? g : QString();
    }
    udp_by_me_ = ini.value("udp/scope", "all").toString() == "me";
    udp_sets_dx_ = ini.value("udp/sets_dx", false).toBool();
    map_hidden_ = ini.value("map_sources/hidden").toStringList();
    for (const char* k : {"dx", "psk", "ota", "hamalert", "aprs", "mesh", "adif"}) {
        const int m = ini.value(QString("map_sources/label_") + k, -1).toInt();
        if (m >= 0 && m <= 3) map_label_.insert(k, m);
    }
    const int mpx = ini.value("map_sources/min_label_px", 0).toInt();
    map_min_label_px_ = mpx >= 0 && mpx <= 200 ? mpx : 0;
    imperial_ = ini.value("units/imperial", false).toBool();
    date_order_ = ini.value("clock/date_order", 0).toInt();
    if (date_order_ < 0 || date_order_ > 2) date_order_ = 0;
    clock_12h_ = ini.value("clock/h12", false).toBool();
    week_sunday_ = ini.value("clock/week_start", 1).toInt() == 0;
    banner_style_ = ini.value("banner/style", 0).toInt();
    if (banner_style_ < 0 || banner_style_ >= kBannerStyles) banner_style_ = 0;
    banner_bg_ = ini.value("banner/background", 0).toInt();
    if (banner_bg_ < 0 || banner_bg_ >= kBannerBackgrounds) banner_bg_ = 0;
    banner_title_ = ini.value("banner/title").toString().trimmed().left(kBannerTitleMax);
    dx_zone_override_ = ini.value("dx/zone").toString().trimmed();
    if (!dx_zone_override_.isEmpty() && !QTimeZone(dx_zone_override_.toUtf8()).isValid()) dx_zone_override_.clear();
    overscan_ = ini.value("display/overscan", 0).toInt();
    if (overscan_ < 0 || overscan_ > 10) overscan_ = 0;
    accent_style_ = ini.value("display/accent", 0).toInt();
    if (accent_style_ < 0 || accent_style_ > 4) accent_style_ = 0;
    text_scale_ = ini.value("display/text_scale", 100).toInt();
    if (text_scale_ != 100 && text_scale_ != 115 && text_scale_ != 130 && text_scale_ != 150) text_scale_ = 100;
    {
        const int idle = ini.value("kiosk/blank_idle", 0).toInt();
        const int off = ini.value("kiosk/blank_off", -1).toInt();
        const int on = ini.value("kiosk/blank_on", -1).toInt();
        const int days = ini.value("kiosk/blank_days", 0x7F).toInt();
        if (blanking_config_valid({idle, off, on, days})) { blank_idle_ = idle; blank_off_ = off; blank_on_ = on; blank_days_ = days; }
    }
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

void AppSettings::setDemoMode(bool on) {
    if (demo_mode_ == on) return;
    demo_mode_ = on;
    emit demoModeChanged();
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
    if (!prog.isEmpty() && prog != "POTA" && prog != "SOTA" && prog != "WWFF") return "Program must be POTA, SOTA, WWFF or empty.";
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

QString AppSettings::saveMesh(const QString& host, int port) {
    static const QRegularExpression host_re("^[A-Za-z0-9.-]{1,253}$");
    const QString h = host.trimmed();
    if (!h.isEmpty() && !host_re.match(h).hasMatch()) return "Meshtastic node may contain only letters, digits, dots and hyphens.";
    if (port <= 0 || port >= 65536) return "Meshtastic port must be 1-65535.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("mesh/host", h);
    ini.setValue("mesh/port", port);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the Meshtastic settings.";
    const bool changed = h != mesh_host_ || port != mesh_port_;
    mesh_host_ = h;
    mesh_port_ = port;
    emit optionsChanged();
    if (changed) emit meshChanged();
    return {};
}

QString AppSettings::savePskMatch(const QString& filter) {
    const QStringList tokens = psk::match_tokens(filter);
    const QString norm = tokens.join(", ");
    if (filter.trimmed().size() > 120) return "PSKReporter filter is too long.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("psk/match", norm);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the PSKReporter filter.";
    psk_match_ = norm;
    emit optionsChanged();
    return {};
}

QString AppSettings::savePskWspr(bool on) {
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("psk/wspr", on);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the WSPR setting.";
    psk_wspr_ = on;
    emit optionsChanged();
    return {};
}

QString AppSettings::savePskRbn(bool on) {
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("psk/rbn", on);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the RBN setting.";
    psk_rbn_ = on;
    emit optionsChanged();
    return {};
}

QString AppSettings::savePskMaxKm(int max_km) {
    if (max_km < 0 || max_km > 20000) return "PSKReporter distance is 0-20000 km (0 = any).";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("psk/max_km", max_km);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the PSKReporter distance.";
    psk_max_km_ = max_km;
    emit optionsChanged();
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

QString AppSettings::saveUdpGroup(const QString& group) {
    const QString g = group.trimmed();
    QHostAddress a;
    if (!g.isEmpty() && !(a.setAddress(g) && a.protocol() == QAbstractSocket::IPv4Protocol && a.isMulticast()))
        return "Multicast group must be empty or an IPv4 address from 224.0.0.0 to 239.255.255.255.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("udp/group", g);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the UDP multicast group.";
    if (udp_group_ != g) {
        udp_group_ = g;
        emit optionsChanged();
    }
    return {};
}

QString AppSettings::saveUdpScope(bool by_me) {
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("udp/scope", by_me ? "me" : "all");
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the UDP spot scope.";
    if (udp_by_me_ != by_me) {
        udp_by_me_ = by_me;
        emit optionsChanged();
    }
    return {};
}

QString AppSettings::saveUdpSetsDx(bool on) {
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("udp/sets_dx", on);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the UDP sets DX option.";
    if (udp_sets_dx_ != on) {
        udp_sets_dx_ = on;
        if (!on) clearDxOverride();
        emit optionsChanged();
    }
    return {};
}

StationSettings AppSettings::persistable() const {
    auto out = station_;
    if (dx_override_) out.dx = saved_dx_;
    return out;
}

bool AppSettings::setDxOverride(double lat, double lon) {
    if (!configured_ || !(lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180)) return false;
    if (!dx_override_) saved_dx_ = station_.dx;
    dx_override_ = true;
    station_.dx = {lat, lon};
    emit stationChanged();
    return true;
}

void AppSettings::clearDxOverride() {
    if (!dx_override_) return;
    dx_override_ = false;
    station_.dx = saved_dx_;
    emit stationChanged();
}

QString AppSettings::saveMapSourceLabel(const QString& key, int mode) {
    static const QStringList kKeys{"dx", "psk", "ota", "hamalert", "aprs", "mesh", "adif"};
    if (!kKeys.contains(key)) return "Unknown map source.";
    if (mode < -1 || mode > 3) return "Unknown label style.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    if (mode < 0) ini.remove("map_sources/label_" + key);
    else ini.setValue("map_sources/label_" + key, mode);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the label style.";
    const int before = map_label_.value(key, -1);
    if (mode < 0) map_label_.remove(key); else map_label_.insert(key, mode);
    if (before != mode) emit optionsChanged();
    return {};
}

QString AppSettings::saveMapMinLabelPx(int px) {
    if (px < 0 || px > 200) return "Label distance must be 0-200 px.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("map_sources/min_label_px", px);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the label distance.";
    if (px != map_min_label_px_) { map_min_label_px_ = px; emit optionsChanged(); }
    return {};
}

QString AppSettings::saveMapSourceVisible(const QString& key, bool visible) {
    static const QStringList kKeys{"dx", "psk", "ota", "hamalert", "aprs", "mesh", "adif"};
    if (!kKeys.contains(key)) return "Unknown map source.";
    QStringList hidden = map_hidden_;
    hidden.removeAll(key);
    if (!visible) hidden << key;
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("map_sources/hidden", hidden);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the map layer setting.";
    if (hidden != map_hidden_) {
        map_hidden_ = hidden;
        emit optionsChanged();
    }
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

QStringList AppSettings::configNames() const { return config_list(config_dir_); }

QString AppSettings::saveConfig(const QString& name) {
    const QString err = config_save(config_dir_, name);
    if (err.isEmpty()) emit configsChanged();
    return err;
}

QString AppSettings::restoreConfig(const QString& name) {
    const QString err = config_restore(config_dir_, name);
    if (!err.isEmpty()) return err;
    loadOptions();
    emit optionsChanged();
    emit panesChanged();
    return {};
}

QString AppSettings::deleteConfig(const QString& name) {
    const QString err = config_delete(config_dir_, name);
    if (err.isEmpty()) emit configsChanged();
    return err;
}

QString AppSettings::renameConfig(const QString& from, const QString& to) {
    const QString err = config_rename(config_dir_, from, to);
    if (err.isEmpty()) emit configsChanged();
    return err;
}

QString AppSettings::saveBannerStyle(int style) {
    if (style < 0 || style >= kBannerStyles) return "Unknown banner colour.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("banner/style", style);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the banner colour.";
    banner_style_ = style;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveBannerBackground(int style) {
    if (style < 0 || style >= kBannerBackgrounds) return "Unknown banner background.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("banner/background", style);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the banner background.";
    banner_bg_ = style;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveBannerTitle(const QString& title) {
    const QString t = title.trimmed().left(kBannerTitleMax);
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    if (t.isEmpty()) ini.remove("banner/title");
    else ini.setValue("banner/title", t);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the banner title.";
    banner_title_ = t;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveDxZone(const QString& zone_id) {
    const QString z = zone_id.trimmed();
    if (!z.isEmpty() && !QTimeZone(z.toUtf8()).isValid()) return "Unknown time zone.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    if (z.isEmpty()) ini.remove("dx/zone");
    else ini.setValue("dx/zone", z);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the DX time zone.";
    dx_zone_override_ = z;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveOverscan(int percent) {
    if (percent < 0 || percent > 10) return "The overscan inset must be between 0 and 10 %.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("display/overscan", percent);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the overscan inset.";
    overscan_ = percent;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveTextScale(int percent) {
    if (percent != 100 && percent != 115 && percent != 130 && percent != 150)
        return "The text size must be 100, 115, 130 or 150 %.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("display/text_scale", percent);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the text size.";
    text_scale_ = percent;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveAccentStyle(int style) {
    if (style < 0 || style > 4) return "The accent colour must be 0..4.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("display/accent", style);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the accent colour.";
    accent_style_ = style;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveBlanking(int idle_minutes, int off_hour, int on_hour) {
    if (!blanking_config_valid({idle_minutes, off_hour, on_hour}))
        return "Idle time must be 0..240 minutes and the screen-off window needs both hours (0..23) or neither.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("kiosk/blank_idle", idle_minutes);
    ini.setValue("kiosk/blank_off", off_hour);
    ini.setValue("kiosk/blank_on", on_hour);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the screen blanking settings.";
    blank_idle_ = idle_minutes;
    blank_off_ = off_hour;
    blank_on_ = on_hour;
    emit optionsChanged();
    return {};
}

QString AppSettings::saveBlankDays(int days) {
    if (!blanking_config_valid({blank_idle_, blank_off_, blank_on_, days})) return "Weekdays must be a 7-bit mask.";
    QSettings ini(config_dir_ + "/options.ini", QSettings::IniFormat);
    ini.setValue("kiosk/blank_days", days);
    ini.sync();
    if (ini.status() != QSettings::NoError) return "Could not save the screen blanking weekdays.";
    blank_days_ = days;
    emit optionsChanged();
    return {};
}

} // namespace hamclock
