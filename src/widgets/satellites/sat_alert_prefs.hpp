#pragma once
// Satellite pass alert preferences: which birds, how high, how early, when not
// to bother. A plain value saved in the options INI ([satalerts]); every change
// returns a new copy so the service can compare old and new.
#include "sat_catalog.hpp"
#include <QList>
#include <QTime>

class QSettings;

namespace hamclock {

constexpr int kDefaultAlertMinElevation = 15;
constexpr int kDefaultAlertLeadMinutes = 5;
constexpr int kMaxAlertLeadMinutes = 60;

struct SatAlertPrefs {
    bool enabled = false;      // master switch, off until the user turns it on
    bool all_workable = true;  // every active/intermittent catalog bird...
    QList<int> muted;          // ...except these NORAD ids
    QList<int> picked;         // plus these (catalog birds outside "all", or TLE-only "other" ones)
    int min_elevation_deg = kDefaultAlertMinElevation; // peak elevation a pass must reach
    int lead_minutes = kDefaultAlertLeadMinutes;
    int quiet_start_hour = -1; // local hours [start, end), may wrap midnight; -1 = no quiet hours
    int quiet_end_hour = -1;
    bool daylight_only = false; // only passes with the Sun up at the station
    bool sound = true;          // in-app chime on desktop; notification sound on Apple
    bool announce_start = true; // in-app banner again at AOS ("up now")

    bool operator==(const SatAlertPrefs& o) const;
    bool operator!=(const SatAlertPrefs& o) const { return !(*this == o); }
};

// Clamped into range (elevation 0-90, lead 0-60, hours -1..23, both or neither
// quiet hours, sorted unique id lists without non-positive ids).
SatAlertPrefs sanitized(const SatAlertPrefs& prefs);

SatAlertPrefs load_sat_alert_prefs(QSettings& ini);
void save_sat_alert_prefs(QSettings& ini, const SatAlertPrefs& prefs);

bool is_alert_selected(const SatAlertPrefs& prefs, const SatCatalog& catalog, int norad);
// Flips one satellite in or out of the selection, keeping "All workable" as it is.
SatAlertPrefs toggle_alert_satellite(const SatAlertPrefs& prefs, const SatCatalog& catalog, int norad);
// Turns "All workable" on or off, keeping individual picks of non-catalog birds.
SatAlertPrefs with_all_workable(const SatAlertPrefs& prefs, bool all);

// True when `local` falls inside the quiet window.
bool in_quiet_hours(const SatAlertPrefs& prefs, const QTime& local);

} // namespace hamclock
