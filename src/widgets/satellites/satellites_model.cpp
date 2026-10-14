#include "satellites_model.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include <QSettings>
#include <QTimeZone>
#include <QVariantMap>
#include <algorithm>
#include <cmath>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// CelesTrak asks for at most one download per group per 2 h; we fetch daily.
constexpr PollPolicy kPolicy{24h, 72h, 6h};
constexpr char kUrl[] = "https://celestrak.org/NORAD/elements/gp.php?GROUP=amateur&FORMAT=tle";
// Amateur-service, alive transmitters only: about 330 kB instead of the whole 4 MB database.
constexpr char kTxUrl[] = "https://db.satnogs.org/api/transmitters/?format=json&service=Amateur&alive=true";
constexpr double kMinElevation = 10.0;
constexpr double kHorizonSeconds = 24 * 3600;
constexpr int kMaxRows = 5;
// Popular FM/linear birds; matched against the TLE name.
const char* const kTracked[] = {"ZARYA", "SO-50", "AO-7", "FO-29", "AO-73", "RS-44", "AO-91", "AO-85"};
} // namespace

QString format_mhz(double hz) { return QString::number(hz / 1e6, 'f', 4) + " MHz"; }

QString compass_point(double az_deg) {
    static const char* const kNames[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                         "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
    const int i = static_cast<int>(std::floor(std::fmod(std::fmod(az_deg, 360.0) + 360.0, 360.0) / 22.5 + 0.5)) % 16;
    return kNames[i];
}

QString satellite_short_name(const QString& tle_name) {
    const QString name = tle_name.trimmed();
    const int open = name.lastIndexOf('(');
    const int close = name.lastIndexOf(')');
    if (open >= 0 && close > open + 1 && name.contains("ZARYA") == false) return name.mid(open + 1, close - open - 1);
    if (name.contains("ZARYA")) return "ISS";
    const int amp = name.indexOf(" &");
    return amp > 0 ? name.left(amp) : name;
}

QString pass_alert_text(const QString& name, qint64 aos, qint64 los, qint64 now, int lead_s) {
    if (now >= aos && now < los) return name + " above horizon";
    if (aos > now && aos - now <= lead_s) return name + QString(" rises in %1 min").arg(std::max<qint64>(1, (aos - now + 30) / 60));
    return QString();
}

std::vector<Tle> fresh_tles(const std::vector<Tle>& tles, double unix_time) {
    std::vector<Tle> out;
    for (const Tle& tle : tles) {
        SatellitePropagator p(tle);
        if (p.valid() && unix_time - p.epoch_unix() <= kMaxTleAgeDays * 86400.0) out.push_back(tle);
    }
    return out;
}

std::vector<Tle> choose_tracked(const std::vector<Tle>& tles, const QStringList& wanted) {
    std::vector<Tle> out;
    for (const QString& w : wanted) {
        for (const Tle& tle : tles) {
            if (satellite_short_name(QString::fromStdString(tle.name)).compare(w.trimmed(), Qt::CaseInsensitive) == 0) {
                out.push_back(tle);
                break;
            }
        }
        if (out.size() >= static_cast<std::size_t>(kMaxTracked)) break;
    }
    return out;
}

OverlayLayer satellite_overlay_layer(const std::vector<Tle>& tles, double unix_time) {
    OverlayLayer layer;
    layer.id = "satellites";
    layer.labels = OverlayLabelMode::Call;
    layer.min_label_distance_px = 30;
    constexpr int kStepS = 180, kBackSteps = 3, kForwardSteps = 17;
    for (const Tle& tle : tles) {
        SatellitePropagator p(tle);
        if (!p.valid()) continue;
        for (int i = -kBackSteps; i <= kForwardSteps; ++i) {
            SubSatellitePoint sp;
            if (i == 0 || !p.sub_point(unix_time + i * kStepS, sp)) continue;
            OverlayMarker m;
            m.where = Location{sp.lat_deg, sp.lon_deg};
            m.color = QColor(i < 0 ? "#7a8a99" : "#4fb0c6");
            m.size_px = 1.5;
            layer.markers.push_back(m);
        }
        SubSatellitePoint now_pt;
        if (!p.sub_point(unix_time, now_pt)) continue;
        OverlayMarker m;
        m.where = Location{now_pt.lat_deg, now_pt.lon_deg};
        m.call = satellite_short_name(QString::fromStdString(tle.name));
        m.color = QColor("#4fb0c6");
        m.shape = OverlayShape::Triangle;
        m.size_px = 5;
        layer.markers.push_back(m);
    }
    return layer;
}

SatellitesModel::SatellitesModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("satellites", "Satellites", parent), settings_(context.settings),
      source_(new PolledSource("celestrak-amateur", QUrl(kUrl), kPolicy, context.transport, this)) {
    // SatNOGS DB (CC BY-SA 4.0): at most daily; optional, the pane works without it.
    tx_source_ = new PolledSource("satnogs-transmitters", QUrl(kTxUrl), kPolicy, context.transport, this);
    connect(tx_source_, &PolledSource::updated, this, &SatellitesModel::load_transmitters);
    detail_timer_.setInterval(1000);
    connect(&detail_timer_, &QTimer::timeout, this, &SatellitesModel::refresh_detail);
    trackSource(*source_, "CelesTrak");
    connect(source_, &PolledSource::updated, this, &SatellitesModel::load);
    timer_.setInterval(30 * 1000);
    connect(&timer_, &QTimer::timeout, this, &SatellitesModel::recompute);
    if (settings_) connect(settings_, &AppSettings::stationChanged, this, &SatellitesModel::recompute);
}

void SatellitesModel::start() { source_->start(); tx_source_->start(); timer_.start(); }

void SatellitesModel::load() {
    tles_.clear();
    for (const Tle& t : parse_tle_set(source_->body().toStdString())) {
        const QString name = QString::fromStdString(t.name);
        for (const char* key : kTracked)
            if (name.contains(QLatin1String(key))) { tles_.push_back(t); break; }
    }
    recompute();
}

void SatellitesModel::recompute() {
    rows_.clear();
    sky_track_.clear();
    listed_.clear();
    alert_.clear();
    const double t = static_cast<double>(now().toSecsSinceEpoch());
    const std::vector<Tle> tles = fresh_tles(tles_, t);
    all_stale_ = !tles_.empty() && tles.empty();
    // Map: the user's choice ([satellites] tracked= in options.ini, at most two), else the two
    // satellites with the soonest passes (the first two in the list without a station).
    QStringList wanted;
    if (settings_) {
        QSettings ini(settings_->optionsPath(), QSettings::IniFormat);
        wanted = ini.value("satellites/tracked").toStringList();
    }
    std::vector<Tle> tracked = choose_tracked(tles, wanted);
    auto publish_layer = [&](const std::vector<Tle>& pick) {
        if (pick.empty()) MapOverlays::instance().remove_layer("satellites");
        else MapOverlays::instance().set_layer(satellite_overlay_layer(pick, t));
    };
    if (tles.empty() || !settings_ || !settings_->configured()) {
        if (tracked.empty()) tracked.assign(tles.begin(), tles.begin() + std::min<std::size_t>(tles.size(), kMaxTracked));
        publish_layer(tracked);
        refresh_detail();
        emit changed();
        return;
    }
    const Observer obs{settings_->deLatitude(), settings_->deLongitude(), 0.0};
    struct Row { Tle tle; SatPass pass; std::vector<SatPass> later; };
    std::vector<Row> found;
    for (const Tle& tle : tles) {
        SatellitePropagator p(tle);
        // Start a few minutes back so a pass in progress is still listed.
        const auto passes = p.passes(obs, t - 900, kHorizonSeconds + 900, kMinElevation, 5);
        for (std::size_t i = 0; i < passes.size(); ++i)
            if (static_cast<double>(passes[i].los) > t) {
                found.push_back({tle, passes[i], std::vector<SatPass>(passes.begin() + i + 1, passes.end())});
                break;
            }
    }
    std::sort(found.begin(), found.end(), [](const Row& a, const Row& b) { return a.pass.aos < b.pass.aos; });
    if (tracked.empty())
        for (std::size_t i = 0; i < found.size() && i < static_cast<std::size_t>(kMaxTracked); ++i) tracked.push_back(found[i].tle);
    publish_layer(tracked);
    if (!found.empty()) {
        constexpr int kSkySamples = 24;
        for (const PassSample& s : SatellitePropagator(found.front().tle).pass_track(obs, found.front().pass, kSkySamples))
            sky_track_.append(QVariantMap{{"az", s.az_deg}, {"el", s.el_deg}});
    }
    if (found.size() > static_cast<std::size_t>(kMaxRows)) found.resize(kMaxRows);
    for (const Row& r : found) listed_.push_back({r.tle, r.pass, r.later});
    for (const Row& r : found) {
        const qint64 until = r.pass.aos - static_cast<qint64>(t);
        QString when;
        if (until <= 0) when = "now";
        else if (until < 90 * 60) when = QString("in %1 min").arg((until + 30) / 60);
        else when = QDateTime::fromSecsSinceEpoch(r.pass.aos, QTimeZone::utc()).toString("HH:mm") + "Z";
        const QString short_name = satellite_short_name(QString::fromStdString(r.tle.name));
        const QString alert = pass_alert_text(short_name, r.pass.aos, r.pass.los, static_cast<qint64>(t));
        if (alert_.isEmpty()) alert_ = alert;
        rows_.append(QVariantMap{{"name", short_name}, {"when", when}, {"maxEl", qRound(r.pass.max_el_deg)},
                                 {"live", until <= 0}, {"soon", !alert.isEmpty()}});
    }
    refresh_detail();
    emit changed();
}

void SatellitesModel::load_transmitters() {
    transmitters_ = group_by_norad(parse_satnogs_transmitters(tx_source_->body().toStdString()));
    refresh_detail();
}

void SatellitesModel::selectRow(int index) {
    selected_ = (index == selected_ || index < 0 || index >= static_cast<int>(listed_.size())) ? -1 : index;
    if (selected_ >= 0) detail_timer_.start(); else detail_timer_.stop();
    refresh_detail();
}

void SatellitesModel::refresh_detail() {
    QVariantMap d;
    if (selected_ >= 0 && selected_ < static_cast<int>(listed_.size()) && settings_ && settings_->configured()) {
        const Listed& l = listed_[selected_];
        const double t = static_cast<double>(now().toSecsSinceEpoch());
        const Observer obs{settings_->deLatitude(), settings_->deLongitude(), 0.0};
        SatellitePropagator p(l.tle);
        LookAngle la;
        if (p.valid() && p.look(t, obs, la)) {
            d["name"] = satellite_short_name(QString::fromStdString(l.tle.name));
            d["az"] = qRound(la.az_deg);
            d["compass"] = compass_point(la.az_deg);
            d["el"] = qRound(la.el_deg);
            d["above"] = la.el_deg >= 0;
            d["rangeKm"] = qRound(la.range_km);
            d["rangeRate"] = QString::number(la.range_rate_km_s, 'f', 2);
            bool lit = true;
            if (p.sunlit(t, lit)) d["sunlit"] = lit;
            auto utc = [](qint64 s) { return QDateTime::fromSecsSinceEpoch(s, QTimeZone::utc()).toString("HH:mm:ss") + "Z"; };
            const auto track = p.pass_track(obs, l.pass, 2);
            d["aos"] = utc(l.pass.aos);
            d["tca"] = utc(l.pass.tca);
            d["los"] = utc(l.pass.los);
            d["maxEl"] = qRound(l.pass.max_el_deg);
            if (track.size() >= 2) {
                d["aosAz"] = compass_point(track.front().az_deg);
                d["losAz"] = compass_point(track.back().az_deg);
            }
            const qint64 until = l.pass.aos - static_cast<qint64>(t);
            d["countdown"] = until <= 0 ? QString("now") : until < 5400 ? QString("in %1 min").arg((until + 30) / 60) : utc(l.pass.aos);
            QVariantList more;
            for (const SatPass& m : l.later)
                more.append(QVariantMap{{"aos", QDateTime::fromSecsSinceEpoch(m.aos, QTimeZone::utc()).toString("dd HH:mm") + "Z"},
                                        {"maxEl", qRound(m.max_el_deg)}});
            d["more"] = more;
            QVariantList tx;
            const auto it = transmitters_.find(tle_norad_id(l.tle));
            if (it != transmitters_.end()) {
                for (const Transmitter& x : it->second) {
                    if (!x.active || tx.size() >= 3) continue;
                    QString down = x.downlink_low_hz > 0 ? format_mhz(doppler_downlink_hz(x.downlink_low_hz, la.range_rate_km_s)) : QString();
                    QString up = x.uplink_low_hz > 0 ? format_mhz(doppler_uplink_hz(x.uplink_low_hz, la.range_rate_km_s)) : QString();
                    QString kind = QString::fromStdString(x.mode);
                    if (x.transponder()) kind += x.inverting ? " inverting" : " linear";
                    if (x.ctcss_hz > 0) kind += QString(" CTCSS %1").arg(x.ctcss_hz, 0, 'f', 1);
                    tx.append(QVariantMap{{"label", QString::fromStdString(x.description)}, {"mode", kind},
                                          {"down", down}, {"up", up}});
                }
            }
            d["transmitters"] = tx;
        }
    }
    if (d != detail_) { detail_ = d; emit detailChanged(); }
}

void register_widget_satellites(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SatellitesModel(context);
    registry.add({"satellites", "Satellites", QUrl("qrc:/widgets/satellites/SatellitesCard.qml"), "strip", 70}, model);
    QTimer::singleShot(0, model, &SatellitesModel::start);
}

} // namespace hamclock
