#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../../domain/astro_satellite.hpp"
#include "../../domain/astro_transmitters.hpp"
#include "../../map/overlay.hpp"
#include <QStringList>
#include <QTimer>
#include <QVariantList>

namespace hamclock {

class PolledSource;
class AppSettings;

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
    // Opens the detail box for listed row `index` (toggles off when already open; -1 closes).
    Q_INVOKABLE void selectRow(int index);

signals:
    void changed();
    void detailChanged();

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
    struct Listed { Tle tle; SatPass pass; };
    std::vector<Listed> listed_;
    std::map<int, std::vector<Transmitter>> transmitters_;
    int selected_ = -1;
    QVariantMap detail_;
    QTimer detail_timer_;
};

// Frequency as "145.8000 MHz" (100 Hz resolution).
QString format_mhz(double hz);
// 16-point compass letters for an azimuth in degrees.
QString compass_point(double az_deg);

// Element sets older than this are dropped: SGP4 drifts by km per day for low orbits.
constexpr int kMaxTleAgeDays = 30;
// Sets whose epoch is within kMaxTleAgeDays of unix_time (or in the near future).
std::vector<Tle> fresh_tles(const std::vector<Tle>& tles, double unix_time);

// At most this many satellites get a ground track on the map.
constexpr int kMaxTracked = 2;
// The sets whose short name matches `wanted` (case-insensitive, in order), at most kMaxTracked.
std::vector<Tle> choose_tracked(const std::vector<Tle>& tles, const QStringList& wanted);

// Short display name: the bracketed designator ("AO-7") if present, else the trimmed name.
QString satellite_short_name(const QString& tle_name);

// Map layer "satellites": each tracked satellite's current sub-point (labelled
// triangle) plus its ground track from -10 to +50 min as unlabelled dots.
OverlayLayer satellite_overlay_layer(const std::vector<Tle>& tles, double unix_time);

void register_widget_satellites(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
