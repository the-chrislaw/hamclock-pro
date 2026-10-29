#include "adsb_model.hpp"
#include "../../platform/apple_lifecycle.hpp"
#include "adsb_parse.hpp"
#include "../../ui/app_settings.hpp"
#include <QNetworkReply>
#include <QSettings>
#include <QNetworkRequest>
#include <cmath>

namespace hamclock {

AdsbModel::AdsbModel(const WidgetContext&, QString url, QObject* parent)
    : WidgetModel("adsb", "ADS-B", parent), url_(std::move(url)) {
    setState("unconfigured", "No PiAware receiver on this device");
    timer_.setInterval(5000);
    connect(&timer_, &QTimer::timeout, this, &AdsbModel::poll);
    auto& lifecycle = AppLifecycle::instance();
    connect(&lifecycle, &AppLifecycle::suspended, this, [this] { timer_.stop(); });
    connect(&lifecycle, &AppLifecycle::resumed, this, [this] { if (started_) { timer_.start(); poll(); } });
}

void AdsbModel::start() { started_ = true; timer_.start(); poll(); }

void AdsbModel::fail(const QString& state, const QString& text) {
    tracked_.reset();
    setState(state, text);
    emit changed();
}

void AdsbModel::poll() {
    if (busy_) return;
    busy_ = true;
    QNetworkRequest req{QUrl(url_)};
    req.setHeader(QNetworkRequest::UserAgentHeader, "HamClockPro (read-only ADS-B client)");
    req.setTransferTimeout(4000);
    auto* reply = net_.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        busy_ = false;
        if (reply->error() != QNetworkReply::NoError) {
            fail("unconfigured", "No PiAware receiver on this device");
            return;
        }
        const auto s = adsb::parse_aircraft(reply->readAll());
        if (!s) { fail("error", "Unreadable aircraft data from the receiver"); return; }
        // A receiver whose clock stopped (decoder hung) must not look live.
        const double skew = std::abs(now().toSecsSinceEpoch() - s->now);
        if (skew > 60) { fail("stale", "Receiver data is out of date"); return; }
        tracked_ = s->tracked;
        positioned_ = s->withPosition;
        setState("live", "Connected", now());
        emit changed();
    });
}

void register_widget_adsb(WidgetRegistry& registry, const WidgetContext& context) {
    QString url = adsb::aircraft_url({}, {});
    if (context.settings) {
        QSettings ini(context.settings->optionsPath(), QSettings::IniFormat);
        url = adsb::aircraft_url(ini.value("adsb/host").toString(), ini.value("adsb/port").toString());
    }
    auto* model = new AdsbModel(context, url);
    // Not listed on iOS (no local station hardware there): never started.
    if (registry.add({"adsb", "ADS-B", QUrl("qrc:/widgets/adsb/AdsbCard.qml"), "strip", 76}, model)) QTimer::singleShot(0, model, &AdsbModel::start);
}

} // namespace hamclock
