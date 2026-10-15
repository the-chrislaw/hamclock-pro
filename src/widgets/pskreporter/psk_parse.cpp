#include "psk_parse.hpp"
#include "../dxcluster/dx_spot_parse.hpp"
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

QString band_summary(const std::vector<std::pair<QString, int>>& stats, int max_bands) {
    QStringList parts;
    for (const auto& [band, n] : stats) {
        if (parts.size() >= max_bands) break;
        parts << QString("%1 %2").arg(band).arg(n);
    }
    return parts.join(QString::fromUtf8(" · "));
}

} // namespace hamclock::psk
