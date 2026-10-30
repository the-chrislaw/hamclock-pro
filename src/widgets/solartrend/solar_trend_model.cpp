#include "solar_trend_model.hpp"
#include "../../net/polled_source.hpp"
#include <QDate>
#include <QTimer>
#include <QVariantMap>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// Polite polling per the roadmap's SWPC policy: the forecast moves every few
// hours, the flux file three times a day, the monthly index once a month.
constexpr PollPolicy kKpPolicy{30min, 6h, 1h};
constexpr PollPolicy kFluxPolicy{6h, 36h, 6h};
constexpr PollPolicy kSsnPolicy{12h, 72h, 12h};
constexpr char kKpUrl[] = "https://services.swpc.noaa.gov/products/noaa-planetary-k-index-forecast.json";
constexpr char kFluxUrl[] = "https://services.swpc.noaa.gov/json/f107_cm_flux.json";
constexpr char kSsnUrl[] = "https://services.swpc.noaa.gov/json/solar-cycle/observed-solar-cycle-indices.json";
} // namespace

SolarTrendModel::SolarTrendModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solartrend", "Solar trend", parent),
      kp_source_(new PolledSource("swpc-kp-forecast", QUrl(kKpUrl), kKpPolicy, context.transport, this)),
      flux_source_(new PolledSource("swpc-f107-history", QUrl(kFluxUrl), kFluxPolicy, context.transport, this)),
      ssn_source_(new PolledSource("swpc-ssn-monthly", QUrl(kSsnUrl), kSsnPolicy, context.transport, this)) {
    for (auto* source : {kp_source_, flux_source_, ssn_source_}) {
        connect(source, &PolledSource::updated, this, [this] { parse(); refreshState(); });
        connect(source, &PolledSource::failed, this, &SolarTrendModel::refreshState);
    }
    connect(this, &WidgetModel::ageChanged, this, &SolarTrendModel::refreshState);
}

void SolarTrendModel::start() {
    for (auto* source : {kp_source_, flux_source_, ssn_source_}) source->start();
}

QString SolarTrendModel::sunspotMonth() const {
    if (!has_ssn_) return {};
    const auto date = QDate::fromString(ssn_.month + "-01", Qt::ISODate);
    return date.isValid() ? QLocale::c().toString(date, "MMM yyyy") : ssn_.month;
}

QString SolarTrendModel::fluxCaption() const {
    if (flux_.size() < 2) return hasFlux() ? "SFI, 10.7 cm" : QString();
    const int delta = qRound(flux_.back().flux) - qRound(flux_.front().flux);
    const auto days = flux_.front().date.daysTo(flux_.back().date);
    return QString("%1%2 over %3 days").arg(delta > 0 ? "+" : "").arg(delta).arg(days);
}

QVariantList SolarTrendModel::fluxHistory() const {
    QVariantList out;
    for (const auto& day : flux_) out.push_back(day.flux);
    return out;
}

QVariantList SolarTrendModel::kpBars() const {
    QVariantList out;
    for (const auto& bar : kp_)
        out.push_back(QVariantMap{{"kp", bar.kp}, {"predicted", bar.predicted},
                                  {"level", solartrend::kp_level(bar.kp)}});
    return out;
}

void SolarTrendModel::parse() {
    kp_ = solartrend::kp_bars(kp_source_->body());
    flux_ = solartrend::flux_daily(flux_source_->body());
    has_ssn_ = solartrend::latest_sunspots(ssn_source_->body(), &ssn_);
    emit valuesChanged();
}

void SolarTrendModel::refreshState() {
    const auto now_utc = now();
    struct Item { PolledSource* source; const char* label; bool has; };
    const Item items[] = {{kp_source_, "Kp", hasKp()}, {flux_source_, "Solar flux", hasFlux()},
                          {ssn_source_, "Sunspots", has_ssn_}};
    QStringList stale, missing;
    QDateTime oldest;
    bool any_error = false;
    QStringList loading;
    for (const auto& item : items) {
        any_error = any_error || !item.source->lastError().isEmpty();
        if (!item.has && awaitingFirstAnswer(*item.source)) { loading << item.label; continue; }
        if (!item.has) { missing << item.label; continue; }
        if (item.source->isStale(now_utc)) stale << item.label;
        if (!oldest.isValid() || item.source->fetchedAt() < oldest) oldest = item.source->fetchedAt();
    }
    if (missing.size() + loading.size() == 3) {
        if (any_error) setState("offline", "NOAA SWPC unreachable: " + kp_source_->lastError());
        else setState("loading", "Contacting NOAA SWPC…");
        return;
    }
    if (!loading.isEmpty() && missing.isEmpty() && stale.isEmpty()) {
        setState("loading", "NOAA SWPC (loading: " + loading.join(", ") + ")", oldest);
        return;
    }
    if (!stale.isEmpty() || !missing.isEmpty()) {
        QStringList parts;
        if (!stale.isEmpty()) parts << "out of date: " + stale.join(", ");
        if (!missing.isEmpty()) parts << "missing: " + missing.join(", ");
        if (!loading.isEmpty()) parts << "loading: " + loading.join(", ");
        setState("stale", "NOAA SWPC (" + parts.join("; ") + ")", oldest);
        return;
    }
    setState("live", "NOAA SWPC", oldest);
}

void register_widget_solartrend(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SolarTrendModel(context);
    registry.add({"solartrend", "Solar trend", QUrl("qrc:/widgets/solartrend/SolarTrendCard.qml"), "strip", 50},
                 model);
    QTimer::singleShot(0, model, &SolarTrendModel::start);
}

} // namespace hamclock
