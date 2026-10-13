#include "solar_alerts_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimeZone>
#include <algorithm>
#include <set>

namespace hamclock::solaralerts {
namespace {
QDateTime parse_swpc_time(const QString& text) {  // "2026 Oct 13 1431 UTC"
    auto time = QDateTime::fromString(text.trimmed().left(16), "yyyy MMM dd HHmm");
    if (!time.isValid()) time = QDateTime::fromString(text.trimmed().left(16), "yyyy MMM d HHmm");
    if (!time.isValid()) return {};
    time.setTimeZone(QTimeZone::utc());
    return time;
}

QString field(const QString& message, const QString& name) {
    const QRegularExpression re("^" + QRegularExpression::escape(name) + ":[ \\t]*(.*?)[ \\t]*$",
                                QRegularExpression::MultilineOption);
    const auto m = re.match(message);
    return m.hasMatch() ? m.captured(1) : QString();
}
} // namespace

std::vector<Alert> parse_alerts(const QByteArray& body) {
    std::vector<Alert> out;
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isArray()) return out;
    static const QRegularExpression headline_re(
        "^(ALERT|EXTENDED WARNING|WARNING|CANCEL WATCH|WATCH|SUMMARY):[ \\t]*(.+?)[ \\t]*$",
        QRegularExpression::MultilineOption);
    static const QRegularExpression scale_re("(?:Noaa Scale|NOAA Scale):\\s*([GSR][1-5])|Category ([GSR][1-5])",
                                             QRegularExpression::CaseInsensitiveOption);
    for (const auto& item : doc.array()) {
        const auto row = item.toObject();
        auto message = row.value("message").toString();
        message.replace("\r\n", "\n");
        const auto head = headline_re.match(message);
        const auto issued = parse_swpc_time(field(message, "Issue Time").remove(" UTC"));
        if (!head.hasMatch() || !issued.isValid() || row.value("product_id").toString().isEmpty()) continue;
        Alert a;
        a.code = row.value("product_id").toString();
        const auto kind = head.captured(1);
        a.cancelled = kind == "CANCEL WATCH";
        a.kind = a.cancelled ? "Cancel" : kind == "EXTENDED WARNING" ? "Extended warning"
                 : kind.left(1) + kind.mid(1).toLower();
        a.headline = head.captured(2);
        a.issued = issued;
        const auto scale = scale_re.match(message);
        if (scale.hasMatch()) a.scale = (scale.captured(1).isEmpty() ? scale.captured(2) : scale.captured(1)).toUpper();
        auto until = field(message, "Now Valid Until");
        if (until.isEmpty()) until = field(message, "Valid To");
        a.until = parse_swpc_time(until.remove(" UTC"));
        out.push_back(a);
    }
    std::stable_sort(out.begin(), out.end(), [](const Alert& x, const Alert& y) { return x.issued > y.issued; });
    return out;
}

std::vector<Alert> active_alerts(const std::vector<Alert>& all, const QDateTime& now) {
    std::vector<Alert> out;
    std::set<QString> seen;
    for (const auto& a : all) {  // newest first: the first message per product wins
        if (a.issued > now || !seen.insert(a.code).second) continue;
        if (a.cancelled) continue;
        if (a.until.isValid()) {
            if (a.until <= now) continue;
        } else {
            const auto window = a.kind == "Watch" ? 48 * 3600 : 24 * 3600;
            if (a.issued.secsTo(now) > window) continue;
        }
        out.push_back(a);
    }
    return out;
}

int alert_level(const Alert& alert) {
    const int n = alert.scale.size() == 2 ? alert.scale.mid(1).toInt() : 0;
    if (n >= 3) return 2;
    if (n >= 1 || alert.kind == "Warning" || alert.kind == "Extended warning" || alert.kind == "Watch") return 1;
    return 0;
}

} // namespace hamclock::solaralerts
