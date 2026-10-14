#include "flrig_parse.hpp"
#include "radio_parse.hpp"
#include <QXmlStreamReader>

namespace hamclock::radio {

QByteArray flrig_request(const char* method) {
    return QByteArray("<?xml version=\"1.0\"?><methodCall><methodName>") + method +
           "</methodName><params></params></methodCall>";
}

std::optional<QString> flrig_scalar(const QByteArray& body) {
    QXmlStreamReader xml(body);
    bool inResponse = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) continue;
        const auto name = xml.name();
        if (name == QLatin1String("methodResponse")) inResponse = true;
        else if (!inResponse) return std::nullopt;
        else if (name == QLatin1String("fault")) return std::nullopt;
        else if (name == QLatin1String("string") || name == QLatin1String("i4") ||
                 name == QLatin1String("int") || name == QLatin1String("double")) {
            const QString text = xml.readElementText().trimmed();
            if (xml.hasError() || text.isEmpty()) return std::nullopt;
            return text;
        }
    }
    return std::nullopt;
}

std::optional<double> flrig_frequency_hz(const QByteArray& body) {
    const auto s = flrig_scalar(body);
    return s ? parse_frequency_hz(*s) : std::nullopt;
}

std::optional<QString> flrig_mode(const QByteArray& body) {
    const auto s = flrig_scalar(body);
    return s ? parse_mode(*s) : std::nullopt;
}

} // namespace hamclock::radio
