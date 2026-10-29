#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../../domain/astro_satellite.hpp"
#include "../../domain/astro_transmitters.hpp"
#include "../../map/overlay.hpp"
#include "../../net/polled_source.hpp"
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <set>

namespace hamclock {

class PolledSource;
class AppSettings;
std::vector<Tle> fresh_tles(const std::vector<Tle>& tles, double unix_time);
// Wraps a transport so a failed GET is retried once at `fallback` (CelesTrak -> AMSAT mirror).
Transport with_fallback(Transport primary, QUrl fallback);
// True for the popular FM/linear birds the pane lists, under CelesTrak or AMSAT naming.
bool is_tracked_satellite(const QString& tle_name);

// Next amateur-satellite passes at the DE station: CelesTrak TLEs (cached,
// refreshed daily) propagated offline with SGP4 from the system clock.
class SatellitesModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY changed)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(bool allStale READ allStale NOTIFY changed)
    Q_PROPERTY(QVariantList skyTrack READ skyTrack NOTIFY changed)
    // Detail box for the tapped row (empty when none): now, next pass, transmitters with Doppler.
    Q_PROPERTY(QVariantMap detail READ detail NOTIFY detailChanged)
    // One-line rise alert ("AO-7 rises in 4 min" / "AO-7 above horizon"); empty when no pass is imminent.
    Q_PROPERTY(QString alert READ alert NOTIFY changed)
    Q_PROPERTY(QString schedule READ schedule NOTIFY changed)
public:
    explicit SatellitesModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();
    void recompute();

    bool hasData() const { return !tles_.empty(); }
    // Soonest first: {name, when ("now" / "in 42 min" / "14:05Z"), maxEl, label}.
    QVariantList rows() const { return rows_; }
    // True when elements exist but every set is older than kMaxTleAgeDays (nothing is shown then).
    bool allStale() const { return all_stale_; }
    // Az/el samples ({az, el} degrees) of the first listed pass, AOS to LOS, for the sky plot; empty if none.
    QVariantList skyTrack() const { return sky_track_; }
    PolledSource& source() { return *source_; }
    PolledSource& transmitterSource() { return *tx_source_; }
    QVariantMap detail() const { return detail_; }
    QString alert() const { return alert_; }
    // Schedule line for all passes in the next 24 h (empty when none); roomy slots show it.
    QString schedule() const { return schedule_; }
    // Opens the detail box for listed row `index` (toggles off when already open; -1 closes).
    Q_INVOKABLE void selectRow(int index);
    // Opens (never toggles) the detail box for the listed satellite whose short name matches the
    // map marker's label, case-insensitively; false when it is not in the list.
    Q_INVOKABLE bool selectByName(const QString& name);
    // Adds the open detail's satellite to the map tracks, or removes it if already tracked; saved
    // to [satellites] tracked= in options.ini (at most kMaxTracked, the oldest choice drops off).
    Q_INVOKABLE void toggleTracked();
    // Sets the saved DX to the open satellite's sub-satellite point now; returns an empty string
    // or a user-facing error. Only on an explicit tap, never automatically.
    Q_INVOKABLE QString setAsDx();
    // The saved `user-tles.txt` text (empty when none).
    Q_INVOKABLE QString userTles() const;
    // Validates and saves pasted two/three-line element sets to `user-tles.txt` beside options.ini
    // and reloads; empty text removes the file. Returns "" on success or a user-facing error
    // (nothing is written then).
    Q_INVOKABLE QString saveUserTles(const QString& text);

    // Fresh element sets and the NORAD ids with SatNOGS transmitters, for the Passes pane.
    std::vector<Tle> elements() const { return fresh_tles(tles_, static_cast<double>(now().toSecsSinceEpoch())); }
    std::set<int> transmitterNorads() const {
        std::set<int> ids;
        for (const auto& kv : transmitters_) ids.insert(kv.first);
        return ids;
    }

signals:
    void changed();
    void detailChanged();
    void elementsChanged();

private:
    void load();
    void load_transmitters();
    void refresh_detail();

    AppSettings* settings_;
    PolledSource* source_;
    PolledSource* tx_source_;
    std::vector<Tle> tles_;
    QTimer timer_;
    QVariantList rows_;
    bool all_stale_ = false;
    QVariantList sky_track_;
    struct Listed { Tle tle; SatPass pass; std::vector<SatPass> later; };
    std::vector<Listed> listed_;
    std::map<int, std::vector<Transmitter>> transmitters_;
    int selected_ = -1;
    QVariantMap detail_;
    QString alert_, schedule_;
    QStringList tracked_names_;   // short names currently drawn on the map
    QTimer detail_timer_;
    QString mutual_key_, mutual_text_;   // cached DE+DX co-visibility line for the open detail
};

// Alert text for a pass: "NAME rises in N min" within lead_s before AOS, "NAME above horizon"
// while it is in progress, empty otherwise.
QString pass_alert_text(const QString& name, qint64 aos, qint64 los, qint64 now, int lead_s = 300);

// One-line schedule of every listed pass: "7 passes in 24 h, best AO-7 78° at 14:05Z"; empty when
// there are none. `passes` holds {short name, pass}; only those ending after `now` and starting
// within `horizon_s` count.
QString pass_schedule_summary(const std::vector<std::pair<QString, SatPass>>& passes, qint64 now, qint64 horizon_s = 24 * 3600);

// Frequency as "145.8000 MHz" (100 Hz resolution).
QString format_mhz(double hz);
// Row frequency text for the first active transmitter with a downlink: "145.9200 MHz FM", or
// "145.9000-146.0000 MHz SSB" for a transponder; empty when none (inactive ones are ignored).
QString primary_downlink_text(const std::vector<Transmitter>& txs);
// 16-point compass letters for an azimuth in degrees.
QString compass_point(double az_deg);

// Element sets older than this are dropped: SGP4 drifts by km per day for low orbits.
constexpr int kMaxTleAgeDays = 30;
// Sets whose epoch is within kMaxTleAgeDays of unix_time (or in the near future).
std::vector<Tle> fresh_tles(const std::vector<Tle>& tles, double unix_time);
// Wraps a transport so a failed GET is retried once at `fallback` (CelesTrak -> AMSAT mirror).
Transport with_fallback(Transport primary, QUrl fallback);
// True for the popular FM/linear birds the pane lists, under CelesTrak or AMSAT naming.
bool is_tracked_satellite(const QString& tle_name);

// At most this many satellites get a ground track on the map.
constexpr int kMaxTracked = 2;
// The sets whose short name matches `wanted` (case-insensitive, in order), at most kMaxTracked.
std::vector<Tle> choose_tracked(const std::vector<Tle>& tles, const QStringList& wanted);

// `current` with `name` removed if present (case-insensitive), else appended; only the last
// kMaxTracked entries are kept.
QStringList toggle_tracked_names(const QStringList& current, const QString& name);

// Short display name: the bracketed designator ("AO-7") if present, else the trimmed name.
QString satellite_short_name(const QString& tle_name);

// Points of the visibility footprint (horizon circle) of a satellite at the given sub-point and
// altitude, evenly spaced by bearing; empty for fewer than 3 points or a non-positive altitude.
std::vector<Location> footprint_ring(double lat_deg, double lon_deg, double alt_km, int points);

// Map layer "satellites": each tracked satellite's current sub-point (labelled
// triangle) plus its ground track from -10 to +50 min as unlabelled dots and a dotted footprint ring.
OverlayLayer satellite_overlay_layer(const std::vector<Tle>& tles, double unix_time);

void register_widget_satellites(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
