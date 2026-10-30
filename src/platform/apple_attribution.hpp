#pragma once
// Data-source credits for the App Store build and the Settings "Data sources"
// card. Platform-free so the list is tested on Linux; QML gets `toVariant()`.
#include <QString>
#include <QVariantList>
#include <vector>

namespace hamclock {

struct AttributionSource {
    QString name;     // who provides the data
    QString used_for; // what the app shows from it
    QString terms;    // licence or credit requirement, in one line
    QString url;      // https page for the provider or its terms
};

// Providers the app fetches from or bundles data of, in display order.
const std::vector<AttributionSource>& attributionSources();

// One map per source with keys name, usedFor, terms, url.
QVariantList attributionModel();

} // namespace hamclock
