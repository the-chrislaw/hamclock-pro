#include "psk_model.hpp"
#include "psk_overlay.hpp"
#include "../../ui/app_settings.hpp"
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
constexpr PollPolicy kPolicy{5min, 20min, 30min};  // PSKReporter: at most every 5 minutes
constexpr int kRecentRows = 6;
} // namespace

PskModel::PskModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("pskreporter", "PSKReporter", parent), settings_(context.settings), transport_(context.transport) {
    if (settings_) {
        connect(settings_, &AppSettings::stationChanged, this, &PskModel::start);
        connect(settings_, &AppSettings::optionsChanged, this, [this] {
            if (!raw_.ok) return;
            applyFilter();
            emit reportsChanged();
            publishOverlay();
        });
    }
}

PskModel::~PskModel() { MapOverlays::instance().remove_layer(psk::kOverlayLayerId); }

void PskModel::setMapVisible(bool visible) {
    map_visible_ = visible;
    publishOverlay();
}

// Drawn only while the feed is live: never stale reports as current.
void PskModel::publishOverlay() {
    if (map_visible_ && state() == "live") MapOverlays::instance().set_layer(psk::overlay_layer(feed_.reports));
    else MapOverlays::instance().remove_layer(psk::kOverlayLayerId);
}

void PskModel::applyFilter() {
    feed_ = raw_;
    if (settings_ && settings_->pskMaxKm() > 0)
        feed_.reports = psk::within_distance(raw_.reports, settings_->deLatitude(), settings_->deLongitude(),
                                             settings_->pskMaxKm());
}

void PskModel::start() {
    const QString call = settings_ ? settings_->callsign().trimmed().toUpper() : QString();
    if (source_ && call == call_) return;
    delete source_;
    source_ = nullptr;
    feed_ = {};
    raw_ = {};
    call_ = call;
    emit reportsChanged();
    publishOverlay();
    const QString url = psk::query_url(call);
    if (url.isEmpty()) {
        setState("unconfigured", call.isEmpty() ? "Not configured: set your callsign in Settings"
                                                : "Not configured: callsign has unsupported characters");
        return;
    }
    source_ = new PolledSource("pskreporter-" + QString(call).replace('/', '-').toLower(), QUrl(url), kPolicy,
                               transport_, this);
    source_->setClock([this] { return now(); });
    connect(source_, &PolledSource::updated, this, [this] {
        raw_ = psk::parse_reports(source_->body());
        applyFilter();
        emit reportsChanged();
        refreshState();
    });
    connect(source_, &PolledSource::failed, this, &PskModel::refreshState);
    connect(this, &WidgetModel::ageChanged, this, &PskModel::refreshState);
    setState("loading", "Contacting PSKReporter…");
    source_->start();
}

QString PskModel::bestSnr() const {
    int best = -999;
    for (const auto& r : feed_.reports)
        if (r.has_snr) best = std::max(best, r.snr_db);
    return best == -999 ? QString() : QString("%1 dB").arg(best);
}

QString PskModel::bands() const { return psk::band_summary(psk::band_stats(feed_.reports), 4); }

QString PskModel::modes() const { return psk::band_summary(psk::mode_stats(feed_.reports), 4); }

QString PskModel::farthest() const {
    if (!settings_) return {};
    const auto f = psk::farthest_report(feed_.reports, settings_->deLatitude(), settings_->deLongitude());
    if (!f.found) return {};
    const bool mi = settings_->imperialUnits();
    const long d = std::lround(mi ? f.km * 0.621371 : f.km);
    return QString("Farthest: %1 %2 %3%4").arg(f.call).arg(d).arg(mi ? "mi" : "km").arg(f.band.isEmpty() ? "" : " " + f.band);
}

QVariantList PskModel::recent() const {
    QVariantList out;
    for (const auto& r : feed_.reports) {
        if (out.size() >= kRecentRows) break;
        out.push_back(QVariantMap{{"call", r.receiver},
                                  {"grid", r.receiver_grid},
                                  {"band", r.band},
                                  {"mode", r.mode},
                                  {"snr", r.has_snr ? QString::number(r.snr_db) : QString()},
                                  {"age", formatAge(r.time, now())}});
    }
    return out;
}

void PskModel::refreshState() {
    refreshStateImpl();
    publishOverlay();
}

void PskModel::refreshStateImpl() {
    if (!source_) return;
    if (!feed_.ok) {
        if (!source_->lastError().isEmpty()) setState("offline", "PSKReporter unreachable: " + source_->lastError());
        else if (source_->fetchedAt().isValid()) setState("stale", "PSKReporter returned no usable data");
        return;
    }
    if (source_->isStale(now())) setState("stale", "PSKReporter (out of date)", source_->fetchedAt());
    else setState("live", "Heard by others, last 15 min", source_->fetchedAt());
}

void register_widget_pskreporter(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new PskModel(context);
    registry.add({"pskreporter", "PSKReporter", QUrl("qrc:/widgets/pskreporter/PskCard.qml"), "strip", 70}, model);
    QTimer::singleShot(0, model, &PskModel::start);
}

} // namespace hamclock
