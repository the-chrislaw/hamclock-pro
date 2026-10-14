#include "bands_matrix_model.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include "../bands/path_model.hpp"
#include "../spaceweather/swpc_parse.hpp"
#include <QSettings>
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// Same products and cadence as the Band conditions widget (roadmap data source policy).
constexpr PollPolicy kKpPolicy{15min, 4h, 1h};
constexpr PollPolicy kFluxPolicy{6h, 36h, 6h};
constexpr char kKpUrl[] = "https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json";
constexpr char kFluxUrl[] = "https://services.swpc.noaa.gov/products/summary/10cm-flux.json";
constexpr int kHours = 24;
} // namespace

BandsMatrixModel::BandsMatrixModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("bandsmatrix", "HF path reliability", parent), settings_(context.settings),
      kp_source_(new PolledSource("bandsmatrix-kp", QUrl(kKpUrl), kKpPolicy, context.transport, this)),
      flux_source_(new PolledSource("bandsmatrix-sfi", QUrl(kFluxUrl), kFluxPolicy, context.transport, this)) {
    for (auto* source : {kp_source_, flux_source_}) {
        connect(source, &PolledSource::updated, this, [this] { parse(); refreshState(); });
        connect(source, &PolledSource::failed, this, &BandsMatrixModel::refreshState);
    }
    // The columns start at the current UTC hour, so recompute when the hour turns.
    connect(this, &WidgetModel::ageChanged, this, [this] { recompute(); refreshState(); });
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, [this] { start_hour_ = -1; recompute(); refreshState(); });
}

void BandsMatrixModel::start() {
    kp_source_->start();
    flux_source_->start();
}

QVariantList BandsMatrixModel::rows() const {
    QVariantList out;
    for (std::size_t b = 0; b < cells_.size(); ++b) {
        QVariantList cells;
        for (double v : cells_[b]) cells.push_back(v);
        out.push_back(QVariantMap{{"band", bands_[int(b)]}, {"now", cells_[b].empty() ? 0.0 : cells_[b].front()},
                                  {"muf", muf_[b]}, {"cells", cells}});
    }
    return out;
}

void BandsMatrixModel::parse() {
    const auto kp = swpc::latest_kp(kp_source_->body());
    const auto flux = swpc::latest_solar_flux(flux_source_->body());
    // Both indices are needed; a matrix from one would be a guess.
    have_inputs_ = kp && flux;
    if (have_inputs_) { kp_ = kp->value; sfi_ = flux->value; }
    start_hour_ = -1;
    recompute();
}

void BandsMatrixModel::recompute() {
    const bool was = has_matrix_;
    const qint64 hour = now().toSecsSinceEpoch() / 3600;
    if (hour == start_hour_ && was) return;
    has_matrix_ = false;
    cells_.clear();
    muf_.clear();
    bands_.clear();
    best_text_.clear();
    path_text_.clear();
    has_path_ = false;
    if (settings_ && have_inputs_) {
        bands::PathInput path;
        path.de_lat = settings_->deLatitude(); path.de_lon = settings_->deLongitude();
        path.dx_lat = settings_->dxLatitude(); path.dx_lon = settings_->dxLongitude();
        // Optional [bandsmatrix] mode = ssb|cw|digital and power_w = 1..1500 in the options file.
        QString mode_name = "SSB";
        {
            QSettings ini(settings_->optionsPath(), QSettings::IniFormat);
            const QString mode = ini.value("bandsmatrix/mode").toString().trimmed().toLower();
            if (mode == "cw") { path.mode = bands::PathMode::Cw; mode_name = "CW"; }
            else if (mode == "digital") { path.mode = bands::PathMode::Digital; mode_name = "Digital"; }
            bool ok = false;
            const double power = ini.value("bandsmatrix/power_w").toString().toDouble(&ok);
            if (ok && power >= 1 && power <= 1500) path.power_w = power;
        }
        const double km = bands::path_distance_km(path);
        has_path_ = settings_->configured() && km >= 50;  // DX not set (or on top of DE): no path to rate
        if (has_path_) {
            const UtcSeconds start{hour * 3600};
            const auto first = bands::path_outlook(path, start, sfi_, kp_);
            cells_ = bands::path_matrix(path, start, kHours, sfi_, kp_);
            std::size_t best = 0;
            for (std::size_t b = 0; b < first.size(); ++b) {
                bands_ << first[b].band;
                muf_.push_back(first[b].muf_mhz);
                if (first[b].reliability > first[best].reliability) best = b;
            }
            has_matrix_ = true;
            start_hour_ = hour;
            path_text_ = QString("%1 km · %2 %3 W").arg(qRound(km)).arg(mode_name).arg(qRound(path.power_w));
            best_text_ = first[best].reliability < 0.05 ? QString("HF closed now")
                : QString("Best now: %1 %2%").arg(first[best].band).arg(qRound(first[best].reliability * 100));
            start_text_ = QString("%1Z").arg(int(hour % 24), 2, 10, QChar('0'));
        }
    }
    emit matrixChanged();
}

void BandsMatrixModel::refreshState() {
    const auto now_utc = now();
    if (!have_inputs_) {
        if (!kp_source_->lastError().isEmpty() || !flux_source_->lastError().isEmpty())
            setState("offline", "NOAA SWPC unreachable: " +
                     (kp_source_->lastError().isEmpty() ? flux_source_->lastError() : kp_source_->lastError()));
        else if (kp_source_->fetchedAt().isValid() || flux_source_->fetchedAt().isValid())
            setState("stale", "NOAA SWPC returned no usable SFI/Kp data");
        else setState("loading", "Contacting NOAA SWPC…");
        return;
    }
    const auto oldest = std::min(kp_source_->fetchedAt(), flux_source_->fetchedAt());
    if (kp_source_->isStale(now_utc) || flux_source_->isStale(now_utc))
        setState("stale", "Estimated from out-of-date NOAA SWPC indices", oldest);
    else setState("live", "Estimated from NOAA SWPC SFI/Kp (approximate, not VOACAP)", oldest);
}

void register_widget_bandsmatrix(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new BandsMatrixModel(context);
    registry.add({"bandsmatrix", "HF path reliability", QUrl("qrc:/widgets/bandsmatrix/BandsMatrixCard.qml"), "strip", 50}, model);
    QTimer::singleShot(0, model, &BandsMatrixModel::start);
}

} // namespace hamclock
