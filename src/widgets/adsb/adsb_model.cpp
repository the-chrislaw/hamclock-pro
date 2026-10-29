#include "adsb_model.hpp"
#include "../../platform/apple_lifecycle.hpp"
#include "adsb_parse.hpp"
#include "../../ui/app_settings.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <cmath>

namespace hamclock {

AdsbModel::AdsbModel(const WidgetContext&, QString url, QObject* parent)
    : WidgetModel("adsb", "ADS-B", parent), url_(std::move(url)) {
    setState("unconfigured", unreachableText());
    timer_.setInterval(5000);
    connect(&timer_, &QTimer::timeout, this, &AdsbModel::poll);
    auto& lifecycle = AppLifecycle::instance();
    connect(&lifecycle, &AppLifecycle::suspended, this, [this] { timer_.stop(); });
    connect(&lifecycle, &AppLifecycle::resumed, this, [this] { if (started_) { timer_.start(); poll(); } });
}

void AdsbModel::start() { started_ = true; timer_.start(); poll(); }

QString AdsbModel::unreachableText() const {
    return QString("No PiAware receiver at %1 — set the address in Settings").arg(QUrl(url_).authority());
}

void AdsbModel::setUrl(const QString& url) {
    if (url == url_) return;
    if (reply_) { reply_->disconnect(this); reply_->abort(); reply_->deleteLater(); }
    busy_ = false;
    url_ = url;
    fail("unconfigured", unreachableText());
    // Not started (or suspended on iPad): start() / resume poll the new address.
    if (!started_ || AppLifecycle::instance().isSuspended()) return;
    timer_.start();
    poll();
}

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
    reply_ = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        busy_ = false;
        if (reply->error() != QNetworkReply::NoError) {
            fail("unconfigured", unreachableText());
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
    AppSettings* settings = context.settings;
    const auto url_from = [settings] {
        return settings ? adsb::aircraft_url(settings->adsbHost(), QString::number(settings->adsbPort())) : adsb::aircraft_url({}, {});
    };
    auto* model = new AdsbModel(context, url_from());
    if (settings) QObject::connect(settings, &AppSettings::adsbChanged, model, [model, url_from] { model->setUrl(url_from()); });
    // Not listed on iOS (no local station hardware there): never started.
    if (registry.add({"adsb", "ADS-B", QUrl("qrc:/widgets/adsb/AdsbCard.qml"), "strip", 76}, model)) QTimer::singleShot(0, model, &AdsbModel::start);
}

} // namespace hamclock
