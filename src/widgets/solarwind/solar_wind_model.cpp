#include "solar_wind_model.hpp"
#include "../../net/polled_source.hpp"
#include <QCoreApplication>
#include <QPointer>
#include <QStringList>
#include <QThreadPool>
#include <QTimer>
#include <algorithm>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// SWPC refreshes these once a minute, but each fetch is ~3.6 MB (wind + mag), so
// poll every 5 minutes: still well inside the 15 minute "reading is old" limit.
constexpr PollPolicy kPolicy{5min, 15min, 15min};
// A reading older than this is not "now": the feed can be fresh while the
// spacecraft data behind it is not.
constexpr qint64 kMaxSampleAgeSecs = 15 * 60;
constexpr int kHistoryMinutes = 60;
constexpr char kWindUrl[] = "https://services.swpc.noaa.gov/json/rtsw/rtsw_wind_1m.json";
constexpr char kMagUrl[] = "https://services.swpc.noaa.gov/json/rtsw/rtsw_mag_1m.json";
} // namespace

SolarWindModel::SolarWindModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solarwind", "Solar wind", parent),
      wind_source_(new PolledSource("swpc-rtsw-wind", QUrl(kWindUrl), kPolicy, context.transport, this)),
      mag_source_(new PolledSource("swpc-rtsw-mag", QUrl(kMagUrl), kPolicy, context.transport, this)) {
    connect(wind_source_, &PolledSource::updated, this, [this] { parseInBackground(Feed::Wind); });
    connect(mag_source_, &PolledSource::updated, this, [this] { parseInBackground(Feed::Mag); });
    for (auto* source : {wind_source_, mag_source_})
        connect(source, &PolledSource::failed, this, &SolarWindModel::refreshState);
    connect(this, &WidgetModel::ageChanged, this, &SolarWindModel::refreshState);
}

void SolarWindModel::start() {
    wind_source_->start();
    mag_source_->start();
}

QString SolarWindModel::speedText() const { return hasSpeed() ? solarwind::speed_description(speed()) : QString(); }
QString SolarWindModel::bzText() const { return hasBz() ? solarwind::bz_description(bz()) : QString(); }

QVariantList SolarWindModel::toList(const Series& series) {
    QVariantList out;
    for (const auto& p : series) out.push_back(p.value);
    return out;
}

SolarWindModel::Series SolarWindModel::lastHour(Series series) {
    if (series.empty()) return series;
    const auto cutoff = series.back().time.addSecs(-kHistoryMinutes * 60);
    series.erase(series.begin(), std::find_if(series.begin(), series.end(),
                                              [&](const solarwind::Point& p) { return p.time >= cutoff; }));
    return series;
}

// Each body is megabytes of JSON: parse only the feed that changed, on a pool
// thread, so the clock and map keep drawing (Pi). The parser is a pure function of
// its (implicitly shared, read-only) body; the result is applied on the GUI thread,
// and only if no newer body for that feed has been queued since.
void SolarWindModel::parseInBackground(Feed feed) {
    const bool wind = feed == Feed::Wind;
    const QByteArray body = (wind ? wind_source_ : mag_source_)->body();
    const int generation = ++(wind ? wind_generation_ : mag_generation_);
    const char* first_field = wind ? solarwind::kSpeed : solarwind::kBz;
    const char* second_field = wind ? solarwind::kDensity : solarwind::kBt;
    QPointer<SolarWindModel> self(this);
    QThreadPool::globalInstance()->start([self, feed, generation, body, first_field, second_field] {
        Series first = lastHour(solarwind::series(body, first_field));
        Series second = lastHour(solarwind::series(body, second_field));
        // Queued to the application object, which outlives every model; the model
        // itself is only touched there, after checking it still exists.
        QMetaObject::invokeMethod(QCoreApplication::instance(),
            [self, feed, generation, first = std::move(first), second = std::move(second)]() mutable {
                if (self) self->applyParsed(feed, generation, std::move(first), std::move(second));
            }, Qt::QueuedConnection);
    });
}

void SolarWindModel::applyParsed(Feed feed, int generation, Series first, Series second) {
    if (feed == Feed::Wind) {
        if (generation != wind_generation_) return; // a newer body is being parsed
        speed_ = std::move(first);
        density_ = std::move(second);
    } else {
        if (generation != mag_generation_) return;
        bz_ = std::move(first);
        bt_ = std::move(second);
    }
    emit valuesChanged();
    refreshState();
}

void SolarWindModel::refreshState() {
    const auto now_utc = now();
    const bool any = hasSpeed() || hasBz();
    if (!any) {
        if (!wind_source_->lastError().isEmpty() || !mag_source_->lastError().isEmpty())
            setState("offline", "NOAA SWPC unreachable: " +
                     (wind_source_->lastError().isEmpty() ? mag_source_->lastError() : wind_source_->lastError()));
        else if (wind_source_->fetchedAt().isValid() || mag_source_->fetchedAt().isValid())
            setState("stale", "NOAA SWPC returned no usable solar wind data");
        else setState("loading", "Contacting NOAA SWPC…");
        return;
    }
    // Age of the newest observation, not of the download.
    QDateTime newest;
    for (const auto* s : {&speed_, &bz_})
        if (!s->empty() && (!newest.isValid() || s->back().time > newest)) newest = s->back().time;
    QStringList notes;
    if (!hasSpeed()) notes << "missing: speed";
    if (!hasBz()) notes << "missing: Bz";
    if (wind_source_->isStale(now_utc) || mag_source_->isStale(now_utc)) notes << "feed out of date";
    if (newest.secsTo(now_utc) > kMaxSampleAgeSecs) notes << "last reading is old";
    if (!notes.isEmpty()) setState("stale", "NOAA SWPC (" + notes.join("; ") + ")", newest);
    else setState("live", "NOAA SWPC RTSW (L1)", newest);
}

void register_widget_solarwind(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SolarWindModel(context);
    registry.add({"solarwind", "Solar wind", QUrl("qrc:/widgets/solarwind/SolarWindCard.qml"), "strip", 45}, model);
    QTimer::singleShot(0, model, &SolarWindModel::start);
}

} // namespace hamclock
