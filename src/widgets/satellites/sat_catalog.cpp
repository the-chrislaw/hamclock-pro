#include "sat_catalog.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <optional>

namespace hamclock {

namespace {
constexpr int kMaxNorad = 999999;
constexpr int kMaxEntries = 500;
constexpr double kMaxMhz = 11000; // QO-100 downlink is 10.49 GHz
const QStringList kStatuses = {"active", "intermittent", "inactive"};

// A number (channel) or [low, high] (passband); nullopt when present but malformed.
std::optional<FreqRange> parse_freq(const QJsonValue& v) {
    if (v.isUndefined() || v.isNull()) return FreqRange{};
    FreqRange r;
    if (v.isDouble()) {
        r = {v.toDouble(), v.toDouble()};
    } else if (v.isArray() && v.toArray().size() == 2 && v.toArray()[0].isDouble() && v.toArray()[1].isDouble()) {
        r = {v.toArray()[0].toDouble(), v.toArray()[1].toDouble()};
    } else {
        return std::nullopt;
    }
    if (!r.valid() || r.hi_mhz > kMaxMhz) return std::nullopt;
    return r;
}

std::optional<Transponder> parse_transponder(const QJsonValue& v, QString& why) {
    const QJsonObject o = v.toObject();
    Transponder t;
    t.mode = o.value("mode").toString();
    if (!catalog_modes().contains(t.mode)) { why = "unknown mode '" + t.mode + "'"; return std::nullopt; }
    const auto up = parse_freq(o.value("uplink_mhz"));
    const auto down = parse_freq(o.value("downlink_mhz"));
    if (!up || !down || !down->valid()) { why = "bad frequency"; return std::nullopt; }
    t.uplink = *up;
    t.downlink = *down;
    t.ctcss_hz = o.value("ctcss_hz").toDouble(0);
    if (t.ctcss_hz < 0 || t.ctcss_hz > 300) { why = "bad CTCSS tone"; return std::nullopt; }
    t.label = o.value("label").toString();
    return t;
}

std::optional<CatalogEntry> parse_entry(const QJsonObject& o, QString& why) {
    CatalogEntry e;
    e.norad = o.value("norad").toInt(0);
    e.name = o.value("name").toString().trimmed();
    e.tle_name = o.value("tle_name").toString().trimmed();
    e.status = o.value("status").toString();
    e.note = o.value("note").toString();
    if (e.norad <= 0 || e.norad > kMaxNorad) { why = "missing NORAD id"; return std::nullopt; }
    if (e.name.isEmpty()) { why = "missing name"; return std::nullopt; }
    if (!kStatuses.contains(e.status)) { why = "unknown status '" + e.status + "'"; return std::nullopt; }
    for (const QJsonValue& tv : o.value("transponders").toArray()) {
        const auto t = parse_transponder(tv, why);
        if (!t) return std::nullopt;
        e.transponders.append(*t);
    }
    if (e.transponders.isEmpty()) { why = "no transponder (mode)"; return std::nullopt; }
    return e;
}
} // namespace

const QStringList& catalog_modes() {
    static const QStringList modes = {"FM", "Linear", "APRS", "Digital"};
    return modes;
}

const CatalogEntry* SatCatalog::find(int norad) const {
    for (const CatalogEntry& e : entries)
        if (e.norad == norad) return &e;
    return nullptr;
}

CatalogParse parse_sat_catalog(const QByteArray& json) {
    CatalogParse out;
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        out.problems.append("catalog is not a JSON object: " + error.errorString());
        return out;
    }
    const QJsonObject root = doc.object();
    out.catalog.updated = root.value("updated").toString();
    const QJsonArray sats = root.value("satellites").toArray();
    if (sats.size() > kMaxEntries) out.problems.append("too many satellites; extra ignored");
    QSet<int> seen;
    for (int i = 0; i < sats.size() && i < kMaxEntries; ++i) {
        QString why;
        const auto entry = parse_entry(sats[i].toObject(), why);
        const QString who = QString("entry %1 (%2)").arg(i).arg(sats[i].toObject().value("name").toString());
        if (!entry) { out.problems.append(who + ": " + why); continue; }
        if (seen.contains(entry->norad)) { out.problems.append(who + ": duplicate NORAD id"); continue; }
        seen.insert(entry->norad);
        out.catalog.entries.append(*entry);
    }
    return out;
}

const SatCatalog& shipped_sat_catalog() {
    static const SatCatalog catalog = [] {
        QFile file(QStringLiteral(":/satalerts/amateur-satellites.json"));
        if (!file.open(QIODevice::ReadOnly)) return SatCatalog{};
        const CatalogParse parsed = parse_sat_catalog(file.readAll());
        for (const QString& problem : parsed.problems) qWarning("Satellite catalog: %s", qPrintable(problem));
        return parsed.catalog;
    }();
    return catalog;
}

QString format_freq_range(const FreqRange& range) {
    if (!range.valid()) return {};
    const QString lo = QString::number(range.lo_mhz, 'f', 3);
    return range.hi_mhz > range.lo_mhz ? lo + "-" + QString::number(range.hi_mhz, 'f', 3) : lo;
}

QString describe_transponder(const Transponder& t) {
    QString text = t.mode + " " + format_freq_range(t.downlink) + " MHz";
    const bool simplex = t.uplink.valid() && format_freq_range(t.uplink) == format_freq_range(t.downlink);
    if (simplex) return text;
    text += " down";
    if (t.uplink.valid()) text += ", " + format_freq_range(t.uplink) + " up";
    if (t.ctcss_hz > 0) text += ", " + QString::number(t.ctcss_hz, 'f', 1) + " Hz";
    return text;
}

} // namespace hamclock
