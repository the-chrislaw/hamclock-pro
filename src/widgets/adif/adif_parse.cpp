#include "adif_parse.hpp"
#include <QDate>
#include <QRegularExpression>
#include <QTime>

namespace hamclock::adif {

namespace {
QDateTime parse_time(const QString& date, QString time) {
    const auto d = QDate::fromString(date, "yyyyMMdd");
    if (!d.isValid()) return {};
    if (time.size() == 4) time += "00";
    auto t = QTime::fromString(time, "HHmmss");
    if (!t.isValid()) t = QTime(0, 0);
    return QDateTime(d, t, Qt::UTC);
}
} // namespace

QString base_call(const QString& call) {
    const QStringList parts = call.toUpper().split('/', Qt::SkipEmptyParts);
    if (parts.size() <= 1) return parts.value(0);
    QString best;
    for (const auto& p : parts) {
        // The main call has a digit and letters; skip short portable markers.
        bool digit = false;
        for (QChar c : p) digit = digit || c.isDigit();
        if (digit && p.size() >= best.size() && p.size() >= 3) best = p;
    }
    return best.isEmpty() ? parts.first() : best;
}

QString prefix_of(const QString& call) {
    const QStringList parts = call.toUpper().split('/', Qt::SkipEmptyParts);
    const QString base = base_call(call);
    if (parts.size() > 1 && parts.first() != base && parts.first().size() < base.size()) {
        bool digit = false;
        for (QChar c : parts.first()) digit = digit || c.isDigit();
        if (digit) return parts.first();
    }
    static const QRegularExpression re("^([0-9]?[A-Z]+[0-9]+)");
    const auto m = re.match(base);
    return m.hasMatch() ? m.captured(1) : QString();
}

std::vector<Qso> parse(const QString& text, int max_records) {
    std::vector<Qso> out;
    QHash<QString, QString> fields;
    bool in_header = !text.trimmed().startsWith('<');
    int i = 0;
    const int n = text.size();
    while (i < n && int(out.size()) < max_records) {
        const int open = text.indexOf('<', i);
        if (open < 0) break;
        const int close = text.indexOf('>', open);
        if (close < 0) break;
        const QStringList spec = text.mid(open + 1, close - open - 1).split(':');
        const QString name = spec.value(0).trimmed().toUpper();
        i = close + 1;
        if (name == "EOH") { in_header = false; fields.clear(); continue; }
        if (name == "EOR") {
            if (!in_header && !fields.value("CALL").isEmpty()) {
                Qso q;
                q.call = fields.value("CALL").toUpper();
                q.band = fields.value("BAND").toLower();
                q.mode = fields.value("MODE").toUpper();
                q.grid = fields.value("GRIDSQUARE").toUpper();
                q.time = parse_time(fields.value("QSO_DATE"), fields.value("TIME_ON"));
                out.push_back(std::move(q));
            }
            fields.clear();
            continue;
        }
        bool ok = false;
        const int len = spec.value(1).toInt(&ok);
        if (!ok || len < 0 || len > 4096 || i + len > n) continue;  // malformed tag: skip it
        if (!in_header) fields.insert(name, text.mid(i, len).trimmed());
        i += len;
    }
    return out;
}

} // namespace hamclock::adif
