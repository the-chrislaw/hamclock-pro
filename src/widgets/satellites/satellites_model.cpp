#include "satellites_model.hpp"
#include "../../net/polled_source.hpp"
#include "../../ui/app_settings.hpp"
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
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

QString pass_schedule_summary(const std::vector<std::pair<QString, SatPass>>& passes, qint64 now, qint64 horizon_s) {
    int count = 0;
    const std::pair<QString, SatPass>* best = nullptr;
    for (const auto& p : passes) {
        if (p.second.los <= now || p.second.aos >= now + horizon_s) continue;
        ++count;
        if (!best || p.second.max_el_deg > best->second.max_el_deg) best = &p;
    }
    if (!best) return {};
    return QString("%1 pass%2 in 24 h, best %3 %4° at %5Z").arg(count).arg(count == 1 ? "" : "es").arg(best->first)
        .arg(qRound(best->second.max_el_deg))
        .arg(QDateTime::fromSecsSinceEpoch(best->second.tca, QTimeZone::utc()).toString("HH:mm"));
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

QStringList toggle_tracked_names(const QStringList& current, const QString& name) {
    QStringList out;
    bool removed = false;
    for (const QString& c : current) {
        if (c.compare(name, Qt::CaseInsensitive) == 0) removed = true;
        else out << c;
    }
    if (!removed) out << name;
    while (out.size() > kMaxTracked) out.removeFirst();
    return out;
}

std::vector<Location> footprint_ring(double lat_deg, double lon_deg, double alt_km, int points) {
    std::vector<Location> ring;
    constexpr double kRe = 6371.0, kPi = 3.14159265358979323846, kRad = kPi / 180.0;
    if (points < 3 || alt_km <= 0) return ring;
    const double rho = std::acos(kRe / (kRe + alt_km)); // angular radius of the horizon circle
    const double la = lat_deg * kRad, lo = lon_deg * kRad;
    for (int i = 0; i < points; ++i) {
        const double brg = 2 * kPi * i / points;
        const double lat = std::asin(std::sin(la) * std::cos(rho) + std::cos(la) * std::sin(rho) * std::cos(brg));
        double lon = lo + std::atan2(std::sin(brg) * std::sin(rho) * std::cos(la),
                                     std::cos(rho) - std::sin(la) * std::sin(lat));
        lon = std::remainder(lon, 2 * kPi);
        ring.push_back(Location{lat / kRad, lon / kRad});
    }
    return ring;
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
        for (const Location& w : footprint_ring(now_pt.lat_deg, now_pt.lon_deg, now_pt.alt_km, 48)) {
            OverlayMarker f;
            f.where = w;
            f.color = QColor("#4fb0c6");
            f.color.setAlpha(120);
            f.size_px = 1;
            layer.markers.push_back(f);
        }
        OverlayMarker m;
        m.where = Location{now_pt.lat_deg, now_pt.lon_deg};
        m.call = satellite_short_name(QString::fromStdString(tle.name));
        m.detail = QString("%1, altitude %2 km").arg(QString::fromStdString(tle.name)).arg(qRound(now_pt.alt_km));
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
    // Optional user-supplied elements (`user-tles.txt` beside options.ini); they win over
    // the download for the same NORAD id and are not limited to the popular-bird list.
    if (settings_) {
        QFile f(QFileInfo(settings_->optionsPath()).absolutePath() + "/user-tles.txt");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            tles_ = merge_user_tles(tles_, parse_tle_set(f.read(1 << 20).toStdString()));
    }
    recompute();
}

namespace {
QString user_tles_path(const AppSettings* settings) {
    return QFileInfo(settings->optionsPath()).absolutePath() + "/user-tles.txt";
}
}

QString SatellitesModel::userTles() const {
    if (!settings_) return {};
    QFile f(user_tles_path(settings_));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(f.read(1 << 20));
}

QString SatellitesModel::saveUserTles(const QString& text) {
    if (!settings_) return "Settings unavailable";
    const QString path = user_tles_path(settings_);
    if (text.trimmed().isEmpty()) {
        QFile::remove(path);
    } else {
        if (text.size() > (1 << 20)) return "Too much text (limit 1 MB)";
        if (parse_tle_set(text.toStdString()).empty())
            return "No valid element sets found (need the two data lines, optionally a name line above)";
        QSaveFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return "Cannot write " + path;
        f.write(text.toUtf8());
        if (!f.commit()) return "Cannot write " + path;
    }
    load();
    return {};
}

void SatellitesModel::recompute() {
    rows_.clear();
    sky_track_.clear();
    listed_.clear();
    alert_.clear();
    schedule_.clear();
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
        tracked_names_.clear();
        for (const Tle& tle : pick) tracked_names_ << satellite_short_name(QString::fromStdString(tle.name));
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
    {
        std::vector<std::pair<QString, SatPass>> all;
        for (const Row& r : found) {
            const QString n = satellite_short_name(QString::fromStdString(r.tle.name));
            all.push_back({n, r.pass});
            for (const SatPass& l : r.later) all.push_back({n, l});
        }
        schedule_ = pass_schedule_summary(all, static_cast<qint64>(t), static_cast<qint64>(kHorizonSeconds));
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

bool SatellitesModel::selectByName(const QString& name) {
    for (int i = 0; i < static_cast<int>(listed_.size()); ++i) {
        if (satellite_short_name(QString::fromStdString(listed_[i].tle.name)).compare(name, Qt::CaseInsensitive) != 0) continue;
        selected_ = i;
        detail_timer_.start();
        refresh_detail();
        return true;
    }
    return false;
}

void SatellitesModel::toggleTracked() {
    if (selected_ < 0 || selected_ >= static_cast<int>(listed_.size()) || !settings_) return;
    const QString name = satellite_short_name(QString::fromStdString(listed_[selected_].tle.name));
    QSettings ini(settings_->optionsPath(), QSettings::IniFormat);
    ini.setValue("satellites/tracked", toggle_tracked_names(tracked_names_, name));
    ini.sync();
    recompute();
}

QString SatellitesModel::setAsDx() {
    if (selected_ < 0 || selected_ >= static_cast<int>(listed_.size()) || !settings_ || !settings_->configured())
        return QStringLiteral("No satellite selected");
    SatellitePropagator p(listed_[selected_].tle);
    SubSatellitePoint sp;
    if (!p.valid() || !p.sub_point(static_cast<double>(now().toSecsSinceEpoch()), sp))
        return QStringLiteral("Position unavailable");
    return settings_->saveStation(settings_->callsign(), settings_->deLatitude(), settings_->deLongitude(),
                                  sp.lat_deg, sp.lon_deg, settings_->deZone());
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
            d["tracked"] = tracked_names_.contains(d["name"].toString(), Qt::CaseInsensitive);
            // Opened in the system browser only; the app never fetches it.
            d["statusUrl"] = QString("https://db.satnogs.org/satellite/%1").arg(tle_norad_id(l.tle));
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
            // Next time the satellite is above the horizon at both DE and DX (cached: 10 min buckets).
            const double dlat = settings_->dxLatitude(), dlon = settings_->dxLongitude();
            if ((dlat != 0.0 || dlon != 0.0) && (dlat != obs.lat_deg || dlon != obs.lon_deg)) {
                const qint64 bucket = static_cast<qint64>(t) / 600;
                const QString key = QString("%1|%2|%3|%4|%5").arg(selected_).arg(bucket).arg(dlat).arg(dlon).arg(l.tle.name.c_str());
                if (key != mutual_key_) {
                    mutual_key_ = key;
                    mutual_text_.clear();
                    const auto w = p.mutual_windows(obs, Observer{dlat, dlon, 0.0}, t, 2 * 86400.0, 0.0, 1);
                    if (!w.empty()) {
                        auto hm = [](qint64 s) { return QDateTime::fromSecsSinceEpoch(s, QTimeZone::utc()).toString("dd HH:mm") + "Z"; };
                        mutual_text_ = QString("%1 > %2 (to DX, max %3°)").arg(hm(w.front().start), hm(w.front().end))
                                           .arg(qRound(w.front().max_min_el_deg));
                    } else mutual_text_ = QStringLiteral("none in 48 h");
                }
                d["mutual"] = mutual_text_;
            }
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
