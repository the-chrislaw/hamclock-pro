// Attribution list: complete entries, https links, no duplicates, required credits present.
#include "../src/platform/apple_attribution.hpp"
#include <QCoreApplication>
#include <QSet>
#include <QVariantMap>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const auto& sources = hamclock::attributionSources();
    check(sources.size() >= 10, "list is populated");
    QSet<QString> names;
    bool complete = true, https = true, unique = true;
    QString all;
    for (const auto& s : sources) {
        complete = complete && !s.name.isEmpty() && !s.used_for.isEmpty() && !s.terms.isEmpty();
        https = https && s.url.startsWith("https://");
        unique = unique && !names.contains(s.name);
        names.insert(s.name);
        all += s.name + ' ' + s.terms + '\n';
    }
    check(complete, "every entry has name, use and terms");
    check(https, "every link is https");
    check(unique, "no duplicate providers");
    check(all.contains("Open-Meteo") && all.contains("CC BY 4.0"), "Open-Meteo credit");
    check(all.contains("HamQSL") && all.contains("N0NBH"), "HamQSL/N0NBH credit");
    check(all.contains("NOAA") && all.contains("NASA") && all.contains("Natural Earth"), "named sources");

    const QVariantList model = hamclock::attributionModel();
    check(model.size() == static_cast<int>(sources.size()), "model matches list");
    const QVariantMap first = model.value(0).toMap();
    check(first.value("name").toString() == sources[0].name && first.contains("usedFor") &&
              first.contains("terms") && first.contains("url"),
          "model keys");
    return failures ? 1 : 0;
}
