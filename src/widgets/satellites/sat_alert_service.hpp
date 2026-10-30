#pragma once
// Satellite pass alerts: predicts the next 48 h of passes at DE for the
// selected satellites (same CelesTrak elements and SGP4 as the Passes pane),
// keeps them scheduled as local notifications on Apple devices, and drives the
// in-app banner everywhere. Exposed to QML as `satAlerts` for Settings.
#include "sat_alert_plan.hpp"
#include "../../domain/astro_satellite.hpp"
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <functional>
#include <set>
#include <vector>

namespace hamclock {

class AppSettings;

class SatAlertService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled NOTIFY prefsChanged)
    Q_PROPERTY(bool allWorkable READ allWorkable NOTIFY prefsChanged)
    Q_PROPERTY(int minElevation READ minElevation NOTIFY prefsChanged)
    Q_PROPERTY(int leadMinutes READ leadMinutes NOTIFY prefsChanged)
    Q_PROPERTY(int quietStart READ quietStart NOTIFY prefsChanged)
    Q_PROPERTY(int quietEnd READ quietEnd NOTIFY prefsChanged)
    Q_PROPERTY(bool daylightOnly READ daylightOnly NOTIFY prefsChanged)
    Q_PROPERTY(bool sound READ sound NOTIFY prefsChanged)
    Q_PROPERTY(bool announceStart READ announceStart NOTIFY prefsChanged)
    // [{mode, sats: [{norad, name, status, note, freq, selected}]}] in catalog_modes() order.
    Q_PROPERTY(QVariantList groups READ groups NOTIFY selectionChanged)
    // Satellites in the TLE set that the catalog does not list: [{norad, name, selected}].
    Q_PROPERTY(QVariantList others READ others NOTIFY selectionChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(QString catalogUpdated READ catalogUpdated CONSTANT)
    // Next few alerts: [{name, when, maxEl, from, to, mode}].
    Q_PROPERTY(QVariantList upcoming READ upcoming NOTIFY planChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY planChanged)
    // Apple: "unknown", "granted" or "denied"; elsewhere "unsupported" (banner only).
    Q_PROPERTY(QString permission READ permission NOTIFY nativeChanged)
    Q_PROPERTY(bool systemNotifications READ systemNotifications CONSTANT)
    // Alerts the system confirms are pending (-1 until it has answered).
    Q_PROPERTY(int scheduledCount READ scheduledCount NOTIFY nativeChanged)
    Q_PROPERTY(QString bannerId READ bannerId NOTIFY bannerChanged)
    Q_PROPERTY(QString bannerText READ bannerText NOTIFY bannerChanged)
    Q_PROPERTY(bool bannerLive READ bannerLive NOTIFY bannerChanged)
public:
    SatAlertService(AppSettings* settings, SatCatalog catalog, QObject* parent = nullptr);

    void setClock(std::function<QDateTime()> clock) { clock_ = std::move(clock); }
    void setTimeZone(const QTimeZone& zone) { zone_override_ = zone; }
    // Desktop sound cue for a new banner (tests swap in a counter).
    void setChime(std::function<void()> chime) { chime_ = std::move(chime); }
    // Fresh elements from the Satellites pane (TLE refresh).
    void setElements(const std::vector<Tle>& tles);
    // Timers, app resume hookup, first prediction.
    void start();

    bool enabled() const { return prefs_.enabled; }
    bool allWorkable() const { return prefs_.all_workable; }
    int minElevation() const { return prefs_.min_elevation_deg; }
    int leadMinutes() const { return prefs_.lead_minutes; }
    int quietStart() const { return prefs_.quiet_start_hour; }
    int quietEnd() const { return prefs_.quiet_end_hour; }
    bool daylightOnly() const { return prefs_.daylight_only; }
    bool sound() const { return prefs_.sound; }
    bool announceStart() const { return prefs_.announce_start; }
    const SatAlertPrefs& prefs() const { return prefs_; }
    QVariantList groups() const;
    QVariantList others() const;
    int selectedCount() const;
    QString catalogUpdated() const { return catalog_.updated; }
    QVariantList upcoming() const;
    QString statusText() const { return status_; }
    QString permission() const { return permission_; }
    bool systemNotifications() const;
    int scheduledCount() const { return scheduled_count_; }
    QString bannerId() const { return banner_.id; }
    QString bannerText() const { return banner_.text; }
    bool bannerLive() const { return banner_.live; }

    Q_INVOKABLE bool isSelected(int norad) const;
    const std::vector<AlertPass>& passes() const { return passes_; }
    const std::vector<PlannedAlert>& plan() const { return plan_; }
    // SGP4 prediction runs so far (tests: settings that only filter must not re-run it).
    int predictionRuns() const { return prediction_runs_; }

    Q_INVOKABLE void setEnabled(bool on);
    Q_INVOKABLE void setAllWorkable(bool all);
    Q_INVOKABLE void toggleSatellite(int norad);
    Q_INVOKABLE void setMinElevation(int degrees);
    Q_INVOKABLE void setLeadMinutes(int minutes);
    Q_INVOKABLE void setQuietHours(int start_hour, int end_hour);
    Q_INVOKABLE void setDaylightOnly(bool on);
    Q_INVOKABLE void setSound(bool on);
    Q_INVOKABLE void setAnnounceStart(bool on);
    // A notification in 5 s (Apple) or the banner now (elsewhere), so the user can check it works.
    Q_INVOKABLE void sendTestAlert();
    Q_INVOKABLE void dismissBanner();
    // Re-reads the permission and pending count from the system (Settings opened, app resumed).
    Q_INVOKABLE void refreshSystemState();

signals:
    void prefsChanged();
    void selectionChanged();
    void planChanged();
    void nativeChanged();
    void bannerChanged();
    // A new banner appeared (sound cue on desktop).
    void alertRaised(const QString& text);

private:
    qint64 nowSecs() const;
    QTimeZone zone() const;
    void apply(const SatAlertPrefs& next);
    void predict();
    QString predictionKey() const;
    void replan();
    void pushToSystem();
    void tick();
    void updateBanner();

    AppSettings* settings_;
    SatCatalog catalog_;
    SatAlertPrefs prefs_;
    std::vector<Tle> tles_;
    std::vector<AlertPass> passes_;
    std::vector<PlannedAlert> plan_;
    QString pushed_signature_;
    qint64 predicted_at_ = 0;
    qint64 test_banner_until_ = 0;
    bool predicted_ = false;
    int prediction_runs_ = 0;
    int elements_version_ = 0;
    std::set<int> predicted_norads_;   // satellites whose passes are in raw_passes_
    QString raw_key_;                  // inputs of raw_passes_ (see predictionKey)
    std::vector<AlertPass> raw_passes_; // every selected pass, horizon to horizon, unfiltered      // a prediction ran (not just "waiting for elements")
    bool push_in_flight_ = false;
    bool push_queued_ = false;
    QString status_;
    QString permission_;
    int scheduled_count_ = -1;
    AlertBanner banner_;
    QStringList dismissed_;
    std::function<QDateTime()> clock_;
    QTimeZone zone_override_;
    std::function<void()> chime_;
    QTimer timer_;
};

} // namespace hamclock
