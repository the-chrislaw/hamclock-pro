#pragma once
// Pure planning for satellite pass alerts: which predicted passes deserve an
// alert, the notification text, the nearest-first budget for iOS's 64 pending
// local notifications, and the in-app banner. No Qt event loop, no platform.
#include "sat_alert_prefs.hpp"
#include <QDateTime>
#include <QStringList>
#include <QTimeZone>
#include <optional>
#include <vector>

namespace hamclock {

// iOS keeps at most 64 pending local notifications per app. Alerts take 60 and
// leave room for the finder's one-off reminders.
constexpr int kAppleMaxPendingNotifications = 64;
constexpr int kMaxScheduledAlerts = 60;
// Identifier prefix of every alert request, so cancelling never touches other reminders.
inline const QString kSatAlertIdPrefix = QStringLiteral("satalert.");

// One predicted pass at the station (AOS/LOS at the horizon).
struct AlertPass {
    int norad = 0;
    QString name;            // display name (catalog name, else the TLE short name)
    qint64 aos = 0, tca = 0, los = 0; // unix seconds
    double max_el_deg = 0;
    QString aos_compass, los_compass;
    double sun_el_deg = 0;   // Sun elevation at the station at TCA
};

struct PlannedAlert {
    QString id;         // satalert.<norad>.<aos minute>: same pass -> same id -> replaced
    int norad = 0;
    qint64 fire = 0;    // unix seconds
    qint64 aos = 0, los = 0;
    QString title;
    QString body;
};

// The passes alerts care about: selected, peak at least min elevation, not
// over yet, AOS outside quiet hours (in `zone`), Sun up if daylight only.
std::vector<AlertPass> alert_passes(const std::vector<AlertPass>& passes, const SatAlertPrefs& prefs,
                                    const SatCatalog& catalog, const QTimeZone& zone, qint64 now);

QString alert_id(int norad, qint64 aos);
QString alert_title(const AlertPass& pass, int lead_minutes);
// "AOS 14:05 · max 54° · NW → SE · FM 437.800 MHz down, 145.990 up, 67.0 Hz"
QString alert_body(const AlertPass& pass, const CatalogEntry* entry, const QTimeZone& zone);

// Notifications to keep pending: one per pass, firing lead minutes before AOS,
// still in the future, nearest first, deduplicated by id, at most `cap`.
// Empty when alerts are off.
std::vector<PlannedAlert> plan_alerts(const std::vector<AlertPass>& passes, const SatAlertPrefs& prefs,
                                      const SatCatalog& catalog, const QTimeZone& zone, qint64 now,
                                      int cap = kMaxScheduledAlerts);

// Pending alert ids (ours, by prefix) that the new plan no longer contains.
// Everything in the plan is (re)added, which replaces same-id requests.
QStringList stale_alert_ids(const QStringList& pending, const std::vector<PlannedAlert>& plan);

// In-app banner (every platform; the only alert on Pi/Linux/Windows/tvOS-less builds).
struct AlertBanner {
    QString id;      // "<alert id>.soon" or "<alert id>.up", so dismissing one does not hide the other
    bool live = false; // true once the satellite is above the horizon
    QString text;
};
// The earliest selected pass within lead time or in progress (when announce_start), not dismissed.
std::optional<AlertBanner> current_banner(const std::vector<AlertPass>& passes, const SatAlertPrefs& prefs,
                                          const SatCatalog& catalog, const QTimeZone& zone, qint64 now,
                                          const QStringList& dismissed);

} // namespace hamclock
