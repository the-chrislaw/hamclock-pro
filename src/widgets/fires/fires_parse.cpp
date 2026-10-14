#include "fires_parse.hpp"
#include <QList>
#include <QTimeZone>
#include <algorithm>
#include <cmath>

namespace hamclock::fires {

Feed parse(const QByteArray& body) {
    Feed feed;
    const auto lines = body.split('\n');
    if (lines.isEmpty()) return feed;
    const auto header = lines[0].trimmed().split(',');
    auto col = [&](const char* name) { return static_cast<int>(header.indexOf(name)); };
    const int lat = col("latitude"), lon = col("longitude"), date = col("acq_date"), time = col("acq_time"),
              conf = col("confidence"), frp = col("frp");
    const int needed = std::max({lat, lon, date, time, conf, frp});
    if (std::min({lat, lon, date, time, conf, frp}) < 0) return feed;
    feed.ok = true;
    for (int i = 1; i < lines.size(); ++i) {
        const auto f = lines[i].trimmed().split(',');
        if (f.size() <= needed) continue;
        bool a = false, b = false, c = false;
        Detection d;
        d.latitude = f[lat].toDouble(&a);
        d.longitude = f[lon].toDouble(&b);
        d.frp_mw = f[frp].toDouble(&c);
        const auto hhmm = f[time].rightJustified(4, '0');
        d.acquired = QDateTime::fromString(QString::fromLatin1(f[date]) + " " + QString::fromLatin1(hhmm), "yyyy-MM-dd HHmm");
        d.acquired.setTimeZone(QTimeZone::utc());
        if (!a || !b || !c || std::abs(d.latitude) > 90 || std::abs(d.longitude) > 180 || d.frp_mw < 0 || !d.acquired.isValid()) continue;
        const auto level = f[conf].toLower();
        d.confidence = level == "high" || level == "h" ? Detection::High : level == "low" || level == "l" ? Detection::Low : Detection::Nominal;
        feed.detections.push_back(d);
    }
    std::stable_sort(feed.detections.begin(), feed.detections.end(),
                     [](const Detection& x, const Detection& y) { return x.frp_mw > y.frp_mw; });
    return feed;
}

} // namespace hamclock::fires
