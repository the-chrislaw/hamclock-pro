#include "sat_alert_plan.hpp"
#include <QSet>
#include <algorithm>
#include <cmath>

namespace hamclock {

namespace {
constexpr qint64 kSecsPerMinute = 60;

QString local_hhmm(qint64 unix_time, const QTimeZone& zone) {
    return QDateTime::fromSecsSinceEpoch(unix_time, zone).toString("HH:mm");
}

QString direction(const AlertPass& p) {
    if (p.aos_compass.isEmpty() || p.los_compass.isEmpty()) return {};
    return p.aos_compass + QStringLiteral(" → ") + p.los_compass;
}

// "FM 437.800 MHz down, 145.990 up, 67.0 Hz" for the bird's first transponder.
QString radio_text(const CatalogEntry* entry) {
    return entry && !entry->transponders.isEmpty() ? describe_transponder(entry->transponders.front()) : QString();
}

// Rounded up so "in 4 min" is never shown when 4 min 30 s remain.
int minutes_until(qint64 from, qint64 to) {
    return static_cast<int>((to - from + kSecsPerMinute - 1) / kSecsPerMinute);
}
} // namespace

std::vector<AlertPass> alert_passes(const std::vector<AlertPass>& passes, const SatAlertPrefs& prefs,
                                    const SatCatalog& catalog, const QTimeZone& zone, qint64 now) {
    std::vector<AlertPass> out;
    if (!prefs.enabled) return out;
    for (const AlertPass& p : passes) {
        if (p.los <= now || p.los <= p.aos) continue;
        if (!is_alert_selected(prefs, catalog, p.norad)) continue;
        if (p.max_el_deg + 1e-9 < prefs.min_elevation_deg) continue;
        if (in_quiet_hours(prefs, QDateTime::fromSecsSinceEpoch(p.aos, zone).time())) continue;
        if (prefs.daylight_only && p.sun_el_deg < 0) continue;
        out.push_back(p);
    }
    std::sort(out.begin(), out.end(), [](const AlertPass& a, const AlertPass& b) { return a.aos < b.aos; });
    return out;
}

QString alert_id(int norad, qint64 aos) {
    return kSatAlertIdPrefix + QString::number(norad) + '.' + QString::number(aos / kSecsPerMinute);
}

QString alert_title(const AlertPass& pass, int lead_minutes) {
    if (lead_minutes <= 0) return pass.name + QStringLiteral(" is rising now");
    return QStringLiteral("%1 pass in %2 min").arg(pass.name).arg(lead_minutes);
}

QString alert_body(const AlertPass& pass, const CatalogEntry* entry, const QTimeZone& zone) {
    QStringList parts = {QStringLiteral("AOS ") + local_hhmm(pass.aos, zone),
                         QStringLiteral("max %1°").arg(qRound(pass.max_el_deg))};
    if (const QString dir = direction(pass); !dir.isEmpty()) parts.append(dir);
    if (const QString radio = radio_text(entry); !radio.isEmpty()) parts.append(radio);
    return parts.join(QStringLiteral(" · "));
}

std::vector<PlannedAlert> plan_alerts(const std::vector<AlertPass>& passes, const SatAlertPrefs& prefs,
                                      const SatCatalog& catalog, const QTimeZone& zone, qint64 now, int cap) {
    std::vector<PlannedAlert> out;
    QSet<QString> ids;
    for (const AlertPass& p : alert_passes(passes, prefs, catalog, zone, now)) {
        if (cap > 0 && static_cast<int>(out.size()) >= cap) break;
        const qint64 fire = p.aos - static_cast<qint64>(prefs.lead_minutes) * kSecsPerMinute;
        if (fire <= now) continue;
        PlannedAlert a;
        a.id = alert_id(p.norad, p.aos);
        if (ids.contains(a.id)) continue;
        ids.insert(a.id);
        a.norad = p.norad;
        a.fire = fire;
        a.aos = p.aos;
        a.los = p.los;
        a.title = alert_title(p, prefs.lead_minutes);
        a.body = alert_body(p, catalog.find(p.norad), zone);
        out.push_back(a);
    }
    // Same lead for every pass, so AOS order is fire order; sort anyway in case that changes.
    std::stable_sort(out.begin(), out.end(), [](const PlannedAlert& a, const PlannedAlert& b) { return a.fire < b.fire; });
    return out;
}

QStringList stale_alert_ids(const QStringList& pending, const std::vector<PlannedAlert>& plan) {
    QSet<QString> keep;
    for (const PlannedAlert& a : plan) keep.insert(a.id);
    QStringList stale;
    for (const QString& id : pending)
        if (id.startsWith(kSatAlertIdPrefix) && !keep.contains(id)) stale.append(id);
    return stale;
}

std::optional<AlertBanner> current_banner(const std::vector<AlertPass>& passes, const SatAlertPrefs& prefs,
                                          const SatCatalog& catalog, const QTimeZone& zone, qint64 now,
                                          const QStringList& dismissed) {
    const qint64 lead = static_cast<qint64>(prefs.lead_minutes) * kSecsPerMinute;
    for (const AlertPass& p : alert_passes(passes, prefs, catalog, zone, now)) {
        const QString base = alert_id(p.norad, p.aos);
        const CatalogEntry* entry = catalog.find(p.norad);
        AlertBanner b;
        if (now < p.aos && now >= p.aos - lead) {
            b.id = base + QStringLiteral(".soon");
            b.text = QStringLiteral("%1 in %2 min · ").arg(p.name).arg(minutes_until(now, p.aos)) +
                     alert_body(p, entry, zone);
        } else if (now >= p.aos && prefs.announce_start) {
            b.id = base + QStringLiteral(".up");
            b.live = true;
            b.text = QStringLiteral("%1 is up now · ").arg(p.name) + alert_body(p, entry, zone);
        } else {
            continue;
        }
        if (!dismissed.contains(b.id)) return b;
    }
    return std::nullopt;
}

} // namespace hamclock
