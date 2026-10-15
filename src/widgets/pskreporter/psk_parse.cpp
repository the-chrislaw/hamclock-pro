#include "psk_parse.hpp"
#include "../dxcluster/dx_spot_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QTimeZone>
#include <QXmlStreamReader>
#include <algorithm>

namespace hamclock::psk {

Feed parse_reports(const QByteArray& body) {
    Feed feed;
    QXmlStreamReader xml(body);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) continue;
        if (xml.name() == QLatin1String("receptionReports")) feed.ok = true;
        if (xml.name() != QLatin1String("receptionReport")) continue;
        const auto a = xml.attributes();
        Report r;
        r.receiver = a.value("receiverCallsign").toString().trimmed().toUpper();
        r.receiver_grid = a.value("receiverLocator").toString().trimmed().toUpper();
        r.sender = a.value("senderCallsign").toString().trimmed().toUpper();
        r.sender_grid = a.value("senderLocator").toString().trimmed().toUpper();
        r.mode = a.value("mode").toString().trimmed().toUpper();
        bool freq_ok = false, time_ok = false;
        r.freq_khz = a.value("frequency").toString().toDouble(&freq_ok) / 1000.0;
        const qint64 secs = a.value("flowStartSeconds").toString().toLongLong(&time_ok);
        if (r.receiver.isEmpty() || !freq_ok || r.freq_khz <= 0 || !time_ok || secs <= 0) continue;
        r.time = QDateTime::fromSecsSinceEpoch(secs, QTimeZone::utc());
        r.band = dxcluster::band_for_khz(r.freq_khz);
        r.snr_db = a.value("sNR").toString().toInt(&r.has_snr);
        feed.reports.push_back(r);
    }
    if (xml.hasError()) return {};
    std::stable_sort(feed.reports.begin(), feed.reports.end(),
                     [](const Report& x, const Report& y) { return x.time > y.time; });
    return feed;
}

Feed parse_wspr(const QByteArray& body) {
    Feed feed;
    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject() || !doc.object().value("data").isArray())
        return feed;
    feed.ok = true;
    for (const auto& v : doc.object().value("data").toArray()) {
        const auto o = v.toObject();
        Report r;
        r.receiver = o.value("rx_sign").toString().trimmed().toUpper();
        r.receiver_grid = o.value("rx_loc").toString().trimmed().toUpper();
        r.sender = o.value("tx_sign").toString().trimmed().toUpper();
        r.sender_grid = o.value("tx_loc").toString().trimmed().toUpper();
        r.mode = "WSPR";
        r.freq_khz = o.value("frequency").toDouble() / 1000.0;
        r.time = QDateTime::fromString(o.value("time").toString(), "yyyy-MM-dd HH:mm:ss");
        r.time.setTimeZone(QTimeZone::utc());
        if (r.receiver.isEmpty() || r.sender.isEmpty() || r.freq_khz <= 0 || !r.time.isValid()) continue;
        r.band = dxcluster::band_for_khz(r.freq_khz);
        r.has_snr = o.value("snr").isDouble();
        r.snr_db = o.value("snr").toInt();
        feed.reports.push_back(r);
    }
    std::stable_sort(feed.reports.begin(), feed.reports.end(),
                     [](const Report& x, const Report& y) { return x.time > y.time; });
    return feed;
}

QString wspr_query_url(const QString& callsign) {
    const QString call = callsign.trimmed().toUpper();
    static const QRegularExpression ok("^[A-Z0-9]+(/[A-Z0-9]+)?$");
    if (!ok.match(call).hasMatch()) return {};
    QString enc = call;
    enc.replace('/', "%2F");
    return "https://db1.wspr.live/?query=SELECT+time,tx_sign,tx_loc,rx_sign,rx_loc,frequency,snr,distance"
           "+FROM+wspr.rx+WHERE+tx_sign%3D'" + enc + "'+AND+time>now()-INTERVAL+1+HOUR"
           "+ORDER+BY+time+DESC+LIMIT+100+FORMAT+JSON";
}

QString query_url(const QString& callsign) {
    const QString call = callsign.trimmed().toUpper();
    static const QRegularExpression ok("^[A-Z0-9]+(/[A-Z0-9]+)?$");
    if (!ok.match(call).hasMatch()) return {};
    QString enc = call;
    enc.replace('/', "%2F");
    // PSKReporter asks for at most one query per 5 minutes and an appcontact.
    return "https://retrieve.pskreporter.info/query?senderCallsign=" + enc +
           "&flowStartSeconds=-900&rronly=1&appcontact=hamclockpro";
}

std::vector<std::pair<QString, int>> band_stats(const std::vector<Report>& reports) {
    std::vector<std::pair<QString, int>> out;
    for (const auto& r : reports) {
        if (r.band.isEmpty()) continue;
        auto it = std::find_if(out.begin(), out.end(), [&](const auto& e) { return e.first == r.band; });
        if (it == out.end()) out.emplace_back(r.band, 1);
        else ++it->second;
    }
    std::stable_sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    return out;
}

std::vector<std::pair<QString, int>> mode_stats(const std::vector<Report>& reports) {
    std::vector<std::pair<QString, int>> out;
    for (const auto& r : reports) {
        const QString mode = r.mode.trimmed().toUpper();
        if (mode.isEmpty()) continue;
        auto it = std::find_if(out.begin(), out.end(), [&](const auto& e) { return e.first == mode; });
        if (it == out.end()) out.emplace_back(mode, 1);
        else ++it->second;
    }
    std::stable_sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    return out;
}

QString band_summary(const std::vector<std::pair<QString, int>>& stats, int max_bands) {
    QStringList parts;
    for (const auto& [band, n] : stats) {
        if (parts.size() >= max_bands) break;
        parts << QString("%1 %2").arg(band).arg(n);
    }
    return parts.join(QString::fromUtf8(" · "));
}

} // namespace hamclock::psk
