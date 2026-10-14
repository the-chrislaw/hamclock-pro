#include "ota_model.hpp"
#include "ota_overlay.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// Activator spots are short lived (minutes); a 2 minute poll keeps the list
// useful without hammering volunteer-run APIs.
constexpr PollPolicy kPolicy{2min, 15min, 30min};
constexpr char kPotaUrl[] = "https://api.pota.app/spot/activator";
constexpr char kSotaUrl[] = "https://api-db2.sota.org.uk/api/spots/50/all/all";
constexpr int kRecentRows = 6;
} // namespace

OtaModel::OtaModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("ota", "On the air", parent),
      pota_(new PolledSource("pota-activator-spots", QUrl(kPotaUrl), kPolicy, context.transport, this)),
      sota_(new PolledSource("sota-spots", QUrl(kSotaUrl), kPolicy, context.transport, this)) {
    connect(pota_, &PolledSource::updated, this, [this] {
        pota_feed_ = ota::parse_pota(pota_->body());
        reparse();
    });
    connect(sota_, &PolledSource::updated, this, [this] {
        sota_feed_ = ota::parse_sota(sota_->body());
        reparse();
    });
    connect(pota_, &PolledSource::failed, this, &OtaModel::refreshState);
    connect(sota_, &PolledSource::failed, this, &OtaModel::refreshState);
    connect(this, &WidgetModel::ageChanged, this, &OtaModel::refreshState);
    if (context.settings) {
        settings_ = context.settings;
        loadSettingsFilter();
        connect(settings_, &AppSettings::optionsChanged, this, &OtaModel::loadSettingsFilter);
    }
}

void OtaModel::loadSettingsFilter() {
    ota::Filter f;
    f.program = settings_->otaProgram();
    f.band = settings_->otaBand();
    f.mode = settings_->otaMode();
    f.max_age_min = settings_->otaMaxAgeMin();
    setFilter(f);
}

void OtaModel::start() {
    pota_->start();
    sota_->start();
}

void OtaModel::reparse() {
    all_spots_ = ota::merge(pota_feed_, sota_feed_);
    applyFilter();
}

void OtaModel::applyFilter() {
    spots_ = ota::filter(all_spots_, filter_, now());
    emit spotsChanged();
    refreshState();
}

void OtaModel::setFilter(const ota::Filter& filter) {
    filter_ = filter;
    applyFilter();
}

QVariantList OtaModel::recent() const {
    QVariantList out;
    for (const auto& s : spots_) {
        if (out.size() >= kRecentRows) break;
        out.push_back(QVariantMap{{"call", s.call},
                                  {"ref", s.reference},
                                  {"freq", ota::format_frequency(s.freq_khz)},
                                  {"mode", s.mode},
                                  {"program", s.program},
                                  {"age", formatAge(s.time, now())}});
    }
    return out;
}

OtaModel::~OtaModel() { MapOverlays::instance().remove_layer(ota::kOverlayLayerId); }

void OtaModel::setMapVisible(bool visible) {
    map_visible_ = visible;
    publishOverlay();
}

// Never keep markers for data that is not live.
void OtaModel::publishOverlay() {
    if (map_visible_ && state() == "live") MapOverlays::instance().set_layer(ota::overlay_layer(spots_));
    else MapOverlays::instance().remove_layer(ota::kOverlayLayerId);
}

void OtaModel::refreshState() {
    refreshStateImpl();
    publishOverlay();
}

void OtaModel::refreshStateImpl() {
    if (!hasData()) {
        const auto err = !pota_->lastError().isEmpty() ? pota_->lastError() : sota_->lastError();
        if (!err.isEmpty()) setState("offline", "POTA/SOTA unreachable: " + err);
        else if (pota_->fetchedAt().isValid() || sota_->fetchedAt().isValid())
            setState("stale", "POTA/SOTA returned no usable spots");
        else setState("loading", "Contacting POTA and SOTA…");
        return;
    }
    // Age of the newest download of any feed that parsed.
    QDateTime fetched;
    bool fresh = false;
    const std::pair<PolledSource*, const ota::Feed*> pairs[] = {{pota_, &pota_feed_}, {sota_, &sota_feed_}};
    for (const auto& pair : pairs) {
        if (!pair.second->ok) continue;
        if (!fetched.isValid() || pair.first->fetchedAt() > fetched) fetched = pair.first->fetchedAt();
        if (!pair.first->isStale(now())) fresh = true;
    }
    QString label = "POTA + SOTA";
    if (!pota_feed_.ok) label = "SOTA only (POTA unavailable)";
    else if (!sota_feed_.ok) label = "POTA only (SOTA unavailable)";
    if (!fresh) setState("stale", label + " (out of date)", fetched);
    else setState("live", label, fetched);
}

void register_widget_ota(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new OtaModel(context);
    registry.add({"ota", "On the air", QUrl("qrc:/widgets/ota/OtaCard.qml"), "strip", 60}, model);
    QTimer::singleShot(0, model, &OtaModel::start);
}

} // namespace hamclock
