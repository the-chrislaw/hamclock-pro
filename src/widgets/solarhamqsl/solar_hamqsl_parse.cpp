#include "solar_hamqsl_parse.hpp"
#include <QLocale>
#include <QTimeZone>
#include <QXmlStreamReader>
#include <algorithm>

namespace hamclock::solarhamqsl {
namespace {
// " 14 Oct 2026 0810 GMT"
QDateTime parse_updated(QString text) {
    text = text.trimmed();
    if (!text.endsWith("GMT")) return {};
    text.chop(3);
    const auto parts = text.trimmed().split(' ', Qt::SkipEmptyParts);  // "14" "Oct" "2026" "0810"
    if (parts.size() != 4 || parts[3].size() != 4) return {};
    const auto date = QLocale::c().toDate(parts[0] + ' ' + parts[1] + ' ' + parts[2], "d MMM yyyy");
    const auto time = QTime(parts[3].left(2).toInt(), parts[3].mid(2).toInt());
    if (!date.isValid() || !time.isValid()) return {};
    QDateTime t(date, time, QTimeZone::utc());
    return t;
}
} // namespace

Report parse_report(const QByteArray& body) {
    Report out;
    QXmlStreamReader xml(body);
    bool in_hf = false, in_vhf = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const auto name = xml.name();
            if (name == QLatin1String("updated")) out.updated = parse_updated(xml.readElementText());
            else if (name == QLatin1String("calculatedconditions")) in_hf = true;
            else if (name == QLatin1String("calculatedvhfconditions")) in_vhf = true;
            else if (in_hf && name == QLatin1String("band")) {
                const auto attrs = xml.attributes();
                const QString band = attrs.value("name").toString();
                const QString time = attrs.value("time").toString();
                const QString word = xml.readElementText().trimmed();
                if (band.isEmpty() || word.isEmpty() || (time != "day" && time != "night")) continue;
                HfBand* row = nullptr;
                for (auto& b : out.hf) if (b.name == band) row = &b;
                if (!row) { out.hf.push_back({band, {}, {}}); row = &out.hf.back(); }
                (time == "day" ? row->day : row->night) = word;
            } else if (in_vhf && name == QLatin1String("phenomenon")) {
                const auto attrs = xml.attributes();
                VhfItem item{attrs.value("name").toString(), attrs.value("location").toString(), {}};
                item.status = xml.readElementText().trimmed();
                if (!item.name.isEmpty() && !item.status.isEmpty()) out.vhf.push_back(item);
            }
        } else if (xml.isEndElement()) {
            if (xml.name() == QLatin1String("calculatedconditions")) in_hf = false;
            else if (xml.name() == QLatin1String("calculatedvhfconditions")) in_vhf = false;
        }
    }
    if (xml.hasError()) return {};
    // A band without both halves is incomplete: drop it rather than show a blank.
    out.hf.erase(std::remove_if(out.hf.begin(), out.hf.end(),
                             [](const HfBand& b) { return b.day.isEmpty() || b.night.isEmpty(); }),
                 out.hf.end());
    return out;
}

int condition_rank(const QString& word) {
    if (word == "Good") return 3;
    if (word == "Fair") return 2;
    if (word == "Poor") return 1;
    return 0;
}

} // namespace hamclock::solarhamqsl
