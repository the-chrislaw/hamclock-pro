#include "sat_alert_service.hpp"
#include "sat_alert_chime.hpp"
#include "satellites_model.hpp"
#include "../../domain/astro_finder.hpp"
#include "../../domain/astro_transmitters.hpp"
#include "../../platform/apple_lifecycle.hpp"
#include "../../platform/apple_notifications.hpp"
#include "../../ui/app_settings.hpp"
#include <QPointer>
#include <QSet>
#include <QSettings>
#include <algorithm>
#include <map>

namespace hamclock {

namespace {
constexpr double kHorizonS = 48 * 3600;
// Search from 3 h back so a pass in progress (banner "up now") is found from its real AOS;
// a pass already up at the search start has a clipped AOS and an unstable id, so it is dropped.
constexpr double kLookBackS = 3 * 3600;
constexpr int kTickMs = 15 * 1000;      // banner countdown resolution
constexpr qint64 kRepredictS = 30 * 60; // the 48 h window slides; refresh the passes this often
constexpr int kUpcomingRows = 5;
constexpr int kTestDelayS = 5;
constexpr qint64 kTestBannerS = 60;
constexpr int kMaxDismissed = 64;
const QString kTestIdPrefix = QStringLiteral("satalerttest."); // not under kSatAlertIdPrefix: replans keep it
const QString kOffSignature = QStringLiteral("off");

bool same_elements(const std::vector<Tle>& a, const std::vector<Tle>& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](const Tle& x, const Tle& y) {
        return x.name == y.name && x.line1 == y.line1 && x.line2 == y.line2;
    });
}

QString day_label(qint64 unix_time, qint64 now, const QTimeZone& zone) {
    const QDateTime t = QDateTime::fromSecsSinceEpoch(unix_time, zone);
    const QDate today = QDateTime::fromSecsSinceEpoch(now, zone).date();
    const QString hhmm = t.toString("HH:mm");
    if (t.date() == today) return "Today " + hhmm;
    if (t.date() == today.addDays(1)) return "Tomorrow " + hhmm;
    return QLocale::c().toString(t.date(), "ddd") + " " + hhmm;
}

QString plan_signature(const std::vector<PlannedAlert>& plan, bool sound) {
    QString s = sound ? "s" : "q";
    for (const PlannedAlert& a : plan) s += a.id + '@' + QString::number(a.fire) + a.title + a.body + '\n';
    return s;
}

std::vector<notify::LocalAlert> to_local(const std::vector<PlannedAlert>& plan, bool sound) {
    std::vector<notify::LocalAlert> out;
    out.reserve(plan.size());
    for (const PlannedAlert& a : plan)
        out.push_back({a.id, QDateTime::fromSecsSinceEpoch(a.fire, QTimeZone::utc()), a.title, a.body, sound});
    return out;
}
// Every pass (horizon to horizon) of the selected satellites in the 48 h window.
std::vector<AlertPass> predict_alert_passes(const std::vector<Tle>& fresh, const std::function<bool(int)>& selected,
                                            const SatCatalog& catalog, const Observer& obs, qint64 now) {
    std::vector<SatellitePropagator> props;
    std::vector<int> norads; // parallel to props
    std::map<int, QString> names;
    props.reserve(fresh.size());
    for (const Tle& tle : fresh) {
        const int norad = tle_norad_id(tle);
        if (!selected(norad) || names.count(norad)) continue;
        const CatalogEntry* entry = catalog.find(norad);
        names[norad] = entry ? entry->name : satellite_short_name(QString::fromStdString(tle.name));
        props.emplace_back(tle);
        norads.push_back(norad);
    }
    // Keyed by NORAD id so two birds with the same short name never mix.
    std::vector<std::pair<std::string, const SatellitePropagator*>> sats;
    for (std::size_t i = 0; i < props.size(); ++i) sats.push_back({std::to_string(norads[i]), &props[i]});
    std::vector<AlertPass> all;
    for (const ScheduledPass& s : pass_schedule(sats, obs, static_cast<double>(now) - kLookBackS,
                                                kHorizonS + kLookBackS, 0.0, PassFilter::All, 0)) {
        if (static_cast<double>(s.pass.pass.aos) <= static_cast<double>(now) - kLookBackS + 1) continue;
        AlertPass p;
        p.norad = std::stoi(s.name);
        p.name = names[p.norad];
        p.aos = s.pass.pass.aos;
        p.tca = s.pass.pass.tca;
        p.los = s.pass.pass.los;
        p.max_el_deg = s.pass.pass.max_el_deg;
        p.aos_compass = QString::fromStdString(s.pass.aos_compass);
        p.los_compass = QString::fromStdString(s.pass.los_compass);
        p.sun_el_deg = s.pass.sun_el_at_tca_deg;
        all.push_back(p);
    }
    return all;
}
} // namespace

SatAlertService::SatAlertService(AppSettings* settings, SatCatalog catalog, QObject* parent)
    : QObject(parent), settings_(settings), catalog_(std::move(catalog)),
      permission_(notify::permission_name(notify::supported() ? notify::Permission::NotDetermined
                                                              : notify::Permission::Unsupported)) {
    if (settings_) {
        QSettings ini(settings_->optionsPath(), QSettings::IniFormat);
        prefs_ = load_sat_alert_prefs(ini);
        // Nothing to cancel at launch when alerts are off: never touch the system centre then.
        if (!prefs_.enabled) pushed_signature_ = kOffSignature;
        connect(settings_, &AppSettings::stationChanged, this, &SatAlertService::predict);
        // A restored saved configuration may carry different alert choices.
        connect(settings_, &AppSettings::optionsChanged, this, [this] {
            QSettings ini(settings_->optionsPath(), QSettings::IniFormat);
            const SatAlertPrefs saved = load_sat_alert_prefs(ini);
            if (saved == prefs_) return;
            prefs_ = saved;
            emit prefsChanged();
            emit selectionChanged();
            predict();
        });
    }
    chime_ = play_alert_chime;
    timer_.setInterval(kTickMs);
    connect(&timer_, &QTimer::timeout, this, &SatAlertService::tick);
}

void SatAlertService::start() {
    // Reschedule on launch and on every return to the foreground: the last plan may have run out.
    const auto reschedule = [this] {
        if (prefs_.enabled) pushed_signature_.clear();
        predict();
        if (prefs_.enabled) refreshSystemState();
    };
    connect(&AppLifecycle::instance(), &AppLifecycle::resumed, this, reschedule);
    timer_.start();
    if (prefs_.enabled && notify::supported()) notify::install_foreground_presenter();
    reschedule();
}

qint64 SatAlertService::nowSecs() const {
    return (clock_ ? clock_() : QDateTime::currentDateTimeUtc()).toSecsSinceEpoch();
}

QTimeZone SatAlertService::zone() const {
    if (zone_override_.isValid()) return zone_override_;
    if (settings_ && !settings_->deZone().isEmpty()) {
        const QTimeZone de(settings_->deZone().toUtf8());
        if (de.isValid()) return de;
    }
    return QTimeZone::systemTimeZone();
}

void SatAlertService::setElements(const std::vector<Tle>& tles) {
    // elementsChanged also fires for transmitter updates; skip the 48 h prediction when nothing moved.
    if (same_elements(tles, tles_)) return;
    tles_ = tles;
    ++elements_version_;
    emit selectionChanged(); // the "other" list follows the TLE set
    predict();
}

void SatAlertService::apply(const SatAlertPrefs& next) {
    const SatAlertPrefs clean = sanitized(next);
    if (clean == prefs_) return;
    const bool turned_on = clean.enabled && !prefs_.enabled;
    prefs_ = clean;
    if (settings_) {
        QSettings ini(settings_->optionsPath(), QSettings::IniFormat);
        save_sat_alert_prefs(ini, prefs_);
    }
    if (turned_on && notify::supported()) notify::install_foreground_presenter();
    emit prefsChanged();
    emit selectionChanged();
    predict();
}

// What the SGP4 prediction depends on: the element set, the station and which satellites are
// selected. Thresholds, lead time, quiet hours and daylight only filter its result.
QString SatAlertService::predictionKey(const std::vector<Tle>& fresh) const {
    QStringList ids;
    for (const Tle& tle : fresh)
        if (const int norad = tle_norad_id(tle); isSelected(norad)) ids.append(QString::number(norad));
    ids.sort();
    return QString("%1|%2,%3|%4").arg(elements_version_).arg(settings_->deLatitude(), 0, 'g', 10)
        .arg(settings_->deLongitude(), 0, 'g', 10).arg(ids.join(','));
}

void SatAlertService::predict() {
    passes_.clear();
    const qint64 now = nowSecs();
    predicted_ = false;
    status_.clear();
    const std::vector<Tle> fresh = fresh_tles(tles_, static_cast<double>(now));
    if (!prefs_.enabled) {
        status_ = "Alerts are off";
    } else if (!settings_ || !settings_->configured()) {
        status_ = "Set your station in Settings to get pass alerts";
    } else if (fresh.empty()) {
        status_ = tles_.empty() ? "Waiting for satellite elements" : "Satellite elements are too old";
    }
    if (!status_.isEmpty()) {
        predicted_at_ = now;
        replan();
        return;
    }
    // Reuse the last run while its inputs are unchanged and its 48 h window is recent enough.
    const QString key = predictionKey(fresh);
    if (key != raw_key_ || now < predicted_at_ || now - predicted_at_ >= kRepredictS) {
        const Observer obs{settings_->deLatitude(), settings_->deLongitude(), 0.0};
        const auto selected = [this](int norad) { return isSelected(norad); };
        raw_passes_ = predict_alert_passes(fresh, selected, catalog_, obs, now);
        raw_key_ = key;
        predicted_at_ = now;
        ++prediction_runs_;
    }
    passes_ = alert_passes(raw_passes_, prefs_, catalog_, zone(), now);
    predicted_ = true;
    replan();
}

void SatAlertService::replan() {
    const qint64 now = nowSecs();
    plan_ = plan_alerts(passes_, prefs_, catalog_, zone(), now);
    if (status_.isEmpty()) {
        status_ = passes_.empty()
            ? QString("No selected passes above %1° in the next 48 h").arg(prefs_.min_elevation_deg)
            : plan_.empty() ? QString("%1 passes in the next 48 h").arg(passes_.size())
            : QString("%1 passes in the next 48 h; alerts set through %2. Opening the app extends them.")
                  .arg(passes_.size()).arg(day_label(plan_.back().aos, now, zone()));
    }
    emit planChanged();
    pushToSystem();
    updateBanner();
}

// One system update at a time: the notification centre answers on background
// queues in no particular order, so a newer plan waits for the one in flight.
void SatAlertService::pushToSystem() {
    if (!notify::supported()) return;
    // Waiting for elements or a station is not a reason to drop alerts already scheduled.
    if (prefs_.enabled && !predicted_) return;
    const QString signature = prefs_.enabled ? plan_signature(plan_, prefs_.sound) : kOffSignature;
    if (signature == pushed_signature_) return;
    if (push_in_flight_) { push_queued_ = true; return; }
    pushed_signature_ = signature;
    push_in_flight_ = true;
    QPointer<SatAlertService> self(this);
    const auto finished = [self](int pending) {
        if (!self) return;
        self->scheduled_count_ = pending;
        self->push_in_flight_ = false;
        emit self->nativeChanged();
        if (self->push_queued_) {
            self->push_queued_ = false;
            self->pushToSystem();
        }
    };
    if (!prefs_.enabled) {
        notify::cancel_alerts(kSatAlertIdPrefix, finished);
        return;
    }
    const std::vector<notify::LocalAlert> alerts = to_local(plan_, prefs_.sound);
    // Asks the first time only; afterwards the system answers at once without a prompt.
    notify::request_permission([self, alerts, finished](notify::Permission p) {
        if (!self) return;
        self->permission_ = notify::permission_name(p);
        if (p == notify::Permission::Granted) {
            notify::replace_alerts(kSatAlertIdPrefix, alerts, finished);
            return;
        }
        self->pushed_signature_.clear(); // try again once the user allows notifications
        finished(-1);
    });
}

void SatAlertService::refreshSystemState() {
    if (!notify::supported()) {
        permission_ = notify::permission_name(notify::Permission::Unsupported);
        emit nativeChanged();
        return;
    }
    QPointer<SatAlertService> self(this);
    notify::query_permission([self](notify::Permission p) {
        if (!self) return;
        self->permission_ = notify::permission_name(p);
        emit self->nativeChanged();
        notify::pending_ids(kSatAlertIdPrefix, [self](const QStringList& ids) {
            if (!self) return;
            self->scheduled_count_ = static_cast<int>(ids.size());
            emit self->nativeChanged();
        });
    });
}

void SatAlertService::tick() {
    if (prefs_.enabled && nowSecs() - predicted_at_ >= kRepredictS) predict();
    else updateBanner();
}

void SatAlertService::updateBanner() {
    const qint64 now = nowSecs();
    if (prefs_.enabled && banner_.id.startsWith(kTestIdPrefix) && now < test_banner_until_) return;
    const auto next = current_banner(passes_, prefs_, catalog_, zone(), now, dismissed_);
    const AlertBanner b = next.value_or(AlertBanner{});
    if (b.id == banner_.id && b.text == banner_.text && b.live == banner_.live) return;
    const bool is_new = b.id != banner_.id && !b.id.isEmpty();
    banner_ = b;
    emit bannerChanged();
    if (is_new) {
        emit alertRaised(b.text);
        if (prefs_.sound && !notify::supported()) if (chime_) chime_();
    }
}

void SatAlertService::dismissBanner() {
    if (banner_.id.isEmpty()) return;
    dismissed_.append(banner_.id);
    while (dismissed_.size() > kMaxDismissed) dismissed_.removeFirst();
    banner_ = {};
    emit bannerChanged();
}

void SatAlertService::sendTestAlert() {
    const QString body = plan_.empty() ? QString("Satellite pass alerts are working.")
                                       : "Next: " + plan_.front().title + " · " + plan_.front().body;
    if (notify::supported()) {
        const qint64 fire = nowSecs() + kTestDelayS;
        const std::vector<notify::LocalAlert> alert = {
            {kTestIdPrefix + QString::number(fire), QDateTime::fromSecsSinceEpoch(fire, QTimeZone::utc()),
             "HamClock Pro test alert", body, prefs_.sound}};
        QPointer<SatAlertService> self(this);
        notify::request_permission([self, alert](notify::Permission p) {
            if (!self) return;
            self->permission_ = notify::permission_name(p);
            emit self->nativeChanged();
            if (p == notify::Permission::Granted) notify::replace_alerts(kTestIdPrefix, alert, {});
        });
        return;
    }
    banner_ = {kTestIdPrefix + QString::number(nowSecs()), false, "Test alert · " + body};
    test_banner_until_ = nowSecs() + kTestBannerS;
    emit bannerChanged();
    emit alertRaised(banner_.text);
    if (prefs_.sound) if (chime_) chime_();
}

bool SatAlertService::systemNotifications() const { return notify::supported(); }
bool SatAlertService::isSelected(int norad) const { return is_alert_selected(prefs_, catalog_, norad); }

void SatAlertService::setEnabled(bool on) { SatAlertPrefs p = prefs_; p.enabled = on; apply(p); }
void SatAlertService::setAllWorkable(bool all) { apply(with_all_workable(prefs_, all)); }
void SatAlertService::toggleSatellite(int norad) { apply(toggle_alert_satellite(prefs_, catalog_, norad)); }
void SatAlertService::setMinElevation(int degrees) { SatAlertPrefs p = prefs_; p.min_elevation_deg = degrees; apply(p); }
void SatAlertService::setLeadMinutes(int minutes) { SatAlertPrefs p = prefs_; p.lead_minutes = minutes; apply(p); }
void SatAlertService::setQuietHours(int start_hour, int end_hour) {
    SatAlertPrefs p = prefs_;
    p.quiet_start_hour = start_hour;
    p.quiet_end_hour = end_hour;
    apply(p);
}
void SatAlertService::setDaylightOnly(bool on) { SatAlertPrefs p = prefs_; p.daylight_only = on; apply(p); }
void SatAlertService::setSound(bool on) { SatAlertPrefs p = prefs_; p.sound = on; apply(p); }
void SatAlertService::setAnnounceStart(bool on) { SatAlertPrefs p = prefs_; p.announce_start = on; apply(p); }

QVariantList SatAlertService::groups() const {
    QVariantList out;
    for (const QString& mode : catalog_modes()) {
        QVariantList sats;
        for (const CatalogEntry& e : catalog_.entries) {
            if (e.primaryMode() != mode) continue;
            QStringList extra;
            for (int i = 1; i < e.transponders.size(); ++i) extra.append(e.transponders[i].mode);
            sats.append(QVariantMap{{"norad", e.norad}, {"name", e.name}, {"status", e.status}, {"note", e.note},
                                    {"freq", describe_transponder(e.transponders.front())},
                                    {"also", extra.join(", ")}, {"selected", isSelected(e.norad)}});
        }
        if (!sats.isEmpty()) out.append(QVariantMap{{"mode", mode}, {"sats", sats}});
    }
    return out;
}

QVariantList SatAlertService::others() const {
    std::map<std::pair<QString, int>, int> by_name;
    QSet<int> seen;
    for (const Tle& tle : tles_) {
        const int norad = tle_norad_id(tle);
        if (norad <= 0 || catalog_.find(norad) || seen.contains(norad)) continue;
        seen.insert(norad);
        by_name[{satellite_short_name(QString::fromStdString(tle.name)), norad}] = norad;
    }
    QVariantList out;
    for (const auto& [key, norad] : by_name)
        out.append(QVariantMap{{"norad", norad}, {"name", key.first}, {"selected", isSelected(norad)}});
    return out;
}

int SatAlertService::selectedCount() const {
    QSet<int> ids;
    for (const CatalogEntry& e : catalog_.entries) if (isSelected(e.norad)) ids.insert(e.norad);
    for (int id : prefs_.picked) if (isSelected(id)) ids.insert(id);
    return static_cast<int>(ids.size());
}

QVariantList SatAlertService::upcoming() const {
    QVariantList out;
    const qint64 now = nowSecs();
    for (const AlertPass& p : passes_) {
        if (out.size() >= kUpcomingRows) break;
        const CatalogEntry* entry = catalog_.find(p.norad);
        out.append(QVariantMap{{"name", p.name}, {"when", p.aos <= now ? QString("now") : day_label(p.aos, now, zone())},
                               {"maxEl", qRound(p.max_el_deg)}, {"from", p.aos_compass}, {"to", p.los_compass},
                               {"mode", entry ? entry->primaryMode() : QString()}});
    }
    return out;
}

} // namespace hamclock
