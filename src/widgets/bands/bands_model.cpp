#include "bands_model.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include "../../map/styles.hpp"
#include "../spaceweather/swpc_parse.hpp"
#include "muf_map.hpp"
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// Same products and cadence as the Space weather widget (roadmap data source policy).
constexpr PollPolicy kKpPolicy{15min, 4h, 1h};
constexpr PollPolicy kFluxPolicy{6h, 36h, 6h};
constexpr char kKpUrl[] = "https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json";
constexpr char kFluxUrl[] = "https://services.swpc.noaa.gov/products/summary/10cm-flux.json";
} // namespace

BandsModel::BandsModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("bands", "Band conditions", parent), settings_(context.settings),
      kp_source_(new PolledSource("bands-kp", QUrl(kKpUrl), kKpPolicy, context.transport, this)),
      flux_source_(new PolledSource("bands-sfi", QUrl(kFluxUrl), kFluxPolicy, context.transport, this)) {
    for (auto* source : {kp_source_, flux_source_}) {
        connect(source, &PolledSource::updated, this, [this] { parse(); refreshState(); });
        connect(source, &PolledSource::failed, this, &BandsModel::refreshState);
    }
    connect(this, &WidgetModel::ageChanged, this, [this] { updatePeriod(); refreshState(); });
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, &BandsModel::updatePeriod);
}

BandsModel::~BandsModel() { MapStyles::instance().remove_style("muf"); }

void BandsModel::start() {
    kp_source_->start();
    flux_source_->start();
}

QVariantList BandsModel::bands() const {
    QVariantList out;
    for (const auto& b : outlook_)
        out.push_back(QVariantMap{{"band", b.band}, {"day", bands::condition_name(b.day)},
                                  {"night", bands::condition_name(b.night)}});
    return out;
}

QString BandsModel::inputsText() const {
    if (!has_outlook_) return {};
    return QString("SFI %1 · Kp %2 · A %3").arg(qRound(sfi_)).arg(kp_, 0, 'f', 1).arg(qRound(a_));
}

void BandsModel::updatePeriod() {
    QString period = "unknown";
    if (settings_) period = bands::period_at(now(), settings_->deLatitude(), settings_->deLongitude());
    if (period != period_) { period_ = period; emit outlookChanged(); }
}

void BandsModel::parse() {
    const auto kp = swpc::latest_kp(kp_source_->body());
    const auto a = bands::latest_a_index(kp_source_->body());
    const auto flux = swpc::latest_solar_flux(flux_source_->body());
    // All three inputs are needed; a partial outlook would be a guess.
    has_outlook_ = kp && a && flux;
    if (has_outlook_) {
        kp_ = kp->value; a_ = a->value; sfi_ = flux->value;
        outlook_ = bands::hf_outlook(sfi_, kp_, a_);
    } else {
        outlook_.clear();
    }
    updatePeriod();
    emit outlookChanged();
}

void BandsModel::refreshState() {
    const auto now_utc = now();
    if (!has_outlook_) {
        if (!kp_source_->lastError().isEmpty() || !flux_source_->lastError().isEmpty())
            setState("offline", "NOAA SWPC unreachable: " +
                     (kp_source_->lastError().isEmpty() ? flux_source_->lastError() : kp_source_->lastError()));
        else if (kp_source_->fetchedAt().isValid() || flux_source_->fetchedAt().isValid())
            setState("stale", "NOAA SWPC returned no usable SFI/Kp/A data");
        else setState("loading", "Contacting NOAA SWPC…");
        return;
    }
    const auto oldest = std::min(kp_source_->fetchedAt(), flux_source_->fetchedAt());
    if (kp_source_->isStale(now_utc) || flux_source_->isStale(now_utc))
        setState("stale", "Estimated from out-of-date NOAA SWPC indices", oldest);
    else setState("live", "Estimated from NOAA SWPC SFI/Kp/A", oldest);
    updateMufStyle(kp_source_->isStale(now_utc) || flux_source_->isStale(now_utc));
}

// Published only from fresh indices; redrawn every 15 minutes as the sun moves.
void BandsModel::updateMufStyle(bool stale) {
    if (!has_outlook_ || stale) {
        MapStyles::instance().remove_style("muf");
        muf_bucket_ = -1;
        return;
    }
    const auto utc = UtcSeconds{now().toSecsSinceEpoch()};
    const qint64 bucket = utc.value / 900;
    if (bucket == muf_bucket_ && sfi_ == muf_sfi_) return;
    muf_bucket_ = bucket;
    muf_sfi_ = sfi_;
    MapStyle style;
    style.id = "muf";
    style.name = "MUF (approx.)";
    style.image = bands::muf_image(sfi_, utc);
    style.opacity = 0.55;
    style.legend = bands::muf_scale();
    style.credit = "Estimated from NOAA SWPC SFI (approximate)";
    MapStyles::instance().set_style(style);
}

void register_widget_bands(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new BandsModel(context);
    registry.add({"bands", "Band conditions", QUrl("qrc:/widgets/bands/BandsCard.qml"), "strip", 50}, model);
    QTimer::singleShot(0, model, &BandsModel::start);
}

} // namespace hamclock
