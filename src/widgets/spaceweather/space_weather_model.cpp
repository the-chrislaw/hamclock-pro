#include "space_weather_model.hpp"
#include "swpc_parse.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// Polling per the roadmap's data source policy: Kp/X-ray every 5–15 min,
// solar flux a few times a day.
constexpr PollPolicy kKpPolicy{15min, 4h, 1h};
constexpr PollPolicy kFluxPolicy{6h, 36h, 6h};
constexpr PollPolicy kXrayPolicy{5min, 30min, 30min};
constexpr char kKpUrl[] = "https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json";
constexpr char kFluxUrl[] = "https://services.swpc.noaa.gov/products/summary/10cm-flux.json";
constexpr char kXrayUrl[] = "https://services.swpc.noaa.gov/json/goes/primary/xrays-6-hour.json";
} // namespace

SpaceWeatherModel::SpaceWeatherModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("spaceweather", "Space weather", parent),
      kp_source_(new PolledSource("swpc-kp", QUrl(kKpUrl), kKpPolicy, context.transport, this)),
      flux_source_(new PolledSource("swpc-sfi", QUrl(kFluxUrl), kFluxPolicy, context.transport, this)),
      xray_source_(new PolledSource("swpc-xray", QUrl(kXrayUrl), kXrayPolicy, context.transport, this)) {
    for (auto* source : {kp_source_, flux_source_, xray_source_}) {
        connect(source, &PolledSource::updated, this, [this] { parse(); refreshState(); });
        connect(source, &PolledSource::failed, this, &SpaceWeatherModel::refreshState);
    }
    connect(this, &WidgetModel::ageChanged, this, &SpaceWeatherModel::refreshState);
}

void SpaceWeatherModel::start() {
    for (auto* source : {kp_source_, flux_source_, xray_source_}) source->start();
}

QString SpaceWeatherModel::kpText() const { return has_kp_ ? swpc::kp_description(kp_) : QString(); }

void SpaceWeatherModel::parse() {
    const auto kp = swpc::latest_kp(kp_source_->body());
    const auto flux = swpc::latest_solar_flux(flux_source_->body());
    const auto xray = swpc::latest_xray_flux(xray_source_->body());
    has_kp_ = kp.has_value();
    kp_ = kp ? kp->value : 0;
    has_sfi_ = flux.has_value();
    sfi_ = flux ? static_cast<int>(std::lround(flux->value)) : 0;
    xray_class_ = xray ? swpc::xray_class(xray->value) : QString();
    emit valuesChanged();
}

// Overall card state: live only when every source is fresh; otherwise report
// the most limiting source so the user knows which value to distrust.
void SpaceWeatherModel::refreshState() {
    const auto now_utc = now();
    struct Item { PolledSource* source; const char* label; bool has; };
    const Item items[] = {{kp_source_, "Kp", has_kp_}, {flux_source_, "Solar flux", has_sfi_},
                          {xray_source_, "X-ray", !xray_class_.isEmpty()}};
    QStringList stale, missing;
    QDateTime oldest;
    for (const auto& item : items) {
        if (!item.has) { missing << item.label; continue; }
        if (item.source->isStale(now_utc)) stale << item.label;
        if (!oldest.isValid() || item.source->fetchedAt() < oldest) oldest = item.source->fetchedAt();
    }
    if (missing.size() == 3) {
        const bool any_error = std::any_of(std::begin(items), std::end(items),
            [](const Item& item) { return !item.source->lastError().isEmpty(); });
        if (any_error) setState("offline", "NOAA SWPC unreachable: " + kp_source_->lastError());
        else setState("loading", "Contacting NOAA SWPC…");
        return;
    }
    if (!stale.isEmpty() || !missing.isEmpty()) {
        QStringList parts;
        if (!stale.isEmpty()) parts << "out of date: " + stale.join(", ");
        if (!missing.isEmpty()) parts << "missing: " + missing.join(", ");
        setState("stale", "NOAA SWPC (" + parts.join("; ") + ")", oldest);
        return;
    }
    setState("live", "NOAA SWPC", oldest);
}

void register_widget_spaceweather(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SpaceWeatherModel(context);
    registry.add({"spaceweather", "Space weather", QUrl("qrc:/widgets/spaceweather/SpaceWeatherCard.qml"),
                  "strip", 40},
                 model);
    QTimer::singleShot(0, model, &SpaceWeatherModel::start);
}

} // namespace hamclock
