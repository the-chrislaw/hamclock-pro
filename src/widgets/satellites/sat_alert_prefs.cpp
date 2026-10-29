#include "sat_alert_prefs.hpp"
#include <QSettings>
#include <QStringList>
#include <algorithm>

namespace hamclock {

namespace {
constexpr int kMaxIds = 1000;

QList<int> clean_ids(QList<int> ids) {
    ids.erase(std::remove_if(ids.begin(), ids.end(), [](int id) { return id <= 0; }), ids.end());
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    if (ids.size() > kMaxIds) ids = ids.mid(0, kMaxIds);
    return ids;
}

// Saved as "25544,27607"; QSettings may also hand back a QStringList for a comma list.
QList<int> read_ids(const QVariant& v) {
    const QStringList parts = v.typeId() == QMetaType::QStringList ? v.toStringList() : v.toString().split(',');
    QList<int> ids;
    for (const QString& part : parts) {
        bool ok = false;
        const int id = part.trimmed().toInt(&ok);
        if (ok) ids.append(id);
    }
    return clean_ids(ids);
}

QString write_ids(const QList<int>& ids) {
    QStringList parts;
    for (int id : ids) parts.append(QString::number(id));
    return parts.join(',');
}

QList<int> without(QList<int> ids, int id) {
    ids.removeAll(id);
    return ids;
}
QList<int> with(QList<int> ids, int id) {
    ids.append(id);
    return clean_ids(ids);
}
} // namespace

bool SatAlertPrefs::operator==(const SatAlertPrefs& o) const {
    return enabled == o.enabled && all_workable == o.all_workable && muted == o.muted && picked == o.picked &&
           min_elevation_deg == o.min_elevation_deg && lead_minutes == o.lead_minutes &&
           quiet_start_hour == o.quiet_start_hour && quiet_end_hour == o.quiet_end_hour &&
           daylight_only == o.daylight_only && sound == o.sound && announce_start == o.announce_start;
}

SatAlertPrefs sanitized(const SatAlertPrefs& prefs) {
    SatAlertPrefs p = prefs;
    p.min_elevation_deg = std::clamp(p.min_elevation_deg, 0, 90);
    p.lead_minutes = std::clamp(p.lead_minutes, 0, kMaxAlertLeadMinutes);
    const auto hour_ok = [](int h) { return h >= 0 && h <= 23; };
    if (!hour_ok(p.quiet_start_hour) || !hour_ok(p.quiet_end_hour) || p.quiet_start_hour == p.quiet_end_hour) {
        p.quiet_start_hour = -1;
        p.quiet_end_hour = -1;
    }
    p.muted = clean_ids(p.muted);
    p.picked = clean_ids(p.picked);
    return p;
}

SatAlertPrefs load_sat_alert_prefs(QSettings& ini) {
    const SatAlertPrefs d;
    SatAlertPrefs p;
    ini.beginGroup("satalerts");
    p.enabled = ini.value("enabled", d.enabled).toBool();
    p.all_workable = ini.value("allWorkable", d.all_workable).toBool();
    p.muted = read_ids(ini.value("muted"));
    p.picked = read_ids(ini.value("picked"));
    p.min_elevation_deg = ini.value("minElevation", d.min_elevation_deg).toInt();
    p.lead_minutes = ini.value("leadMinutes", d.lead_minutes).toInt();
    p.quiet_start_hour = ini.value("quietStart", d.quiet_start_hour).toInt();
    p.quiet_end_hour = ini.value("quietEnd", d.quiet_end_hour).toInt();
    p.daylight_only = ini.value("daylightOnly", d.daylight_only).toBool();
    p.sound = ini.value("sound", d.sound).toBool();
    p.announce_start = ini.value("announceStart", d.announce_start).toBool();
    ini.endGroup();
    return sanitized(p);
}

void save_sat_alert_prefs(QSettings& ini, const SatAlertPrefs& prefs) {
    const SatAlertPrefs p = sanitized(prefs);
    ini.beginGroup("satalerts");
    ini.setValue("enabled", p.enabled);
    ini.setValue("allWorkable", p.all_workable);
    ini.setValue("muted", write_ids(p.muted));
    ini.setValue("picked", write_ids(p.picked));
    ini.setValue("minElevation", p.min_elevation_deg);
    ini.setValue("leadMinutes", p.lead_minutes);
    ini.setValue("quietStart", p.quiet_start_hour);
    ini.setValue("quietEnd", p.quiet_end_hour);
    ini.setValue("daylightOnly", p.daylight_only);
    ini.setValue("sound", p.sound);
    ini.setValue("announceStart", p.announce_start);
    ini.endGroup();
    ini.sync();
}

bool is_alert_selected(const SatAlertPrefs& prefs, const SatCatalog& catalog, int norad) {
    if (norad <= 0 || prefs.muted.contains(norad)) return false;
    if (prefs.picked.contains(norad)) return true;
    const CatalogEntry* entry = catalog.find(norad);
    return prefs.all_workable && entry && entry->workable();
}

SatAlertPrefs toggle_alert_satellite(const SatAlertPrefs& prefs, const SatCatalog& catalog, int norad) {
    if (norad <= 0) return prefs;
    SatAlertPrefs next = prefs;
    if (is_alert_selected(prefs, catalog, norad)) {
        next.picked = without(prefs.picked, norad);
        if (is_alert_selected(next, catalog, norad)) next.muted = with(prefs.muted, norad);
    } else {
        next.muted = without(prefs.muted, norad);
        if (!is_alert_selected(next, catalog, norad)) next.picked = with(prefs.picked, norad);
    }
    return next;
}

SatAlertPrefs with_all_workable(const SatAlertPrefs& prefs, bool all) {
    SatAlertPrefs next = prefs;
    next.all_workable = all;
    if (!all) next.muted.clear(); // mutes only mean something against "all"
    return next;
}

bool in_quiet_hours(const SatAlertPrefs& prefs, const QTime& local) {
    const int s = prefs.quiet_start_hour, e = prefs.quiet_end_hour;
    if (s < 0 || e < 0 || s == e || !local.isValid()) return false;
    const int h = local.hour();
    return s < e ? (h >= s && h < e) : (h >= s || h < e);
}

} // namespace hamclock
