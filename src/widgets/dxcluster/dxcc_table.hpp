#pragma once
// Callsign prefix -> DXCC entity lookup from the AD1C Country Files (cty.dat, MIT licence, see docs/third-party-cty.md).
#include <QHash>
#include <QString>
#include <vector>

namespace hamclock::dxcluster {

struct DxccEntity {
    QString name, continent, primary_prefix;
    int cq_zone = 0, itu_zone = 0;
    double lat = 0, lon = 0; // degrees, east positive (cty.dat stores west positive)
};

class DxccTable {
public:
    // Parses cty.dat text; returns the number of entities (0 for unusable input). Replaces any previous content.
    int parse(const QString& text);
    int size() const { return int(entities_.size()); }
    // Entity for a callsign (portable forms handled: EA8/K1ABC, K1ABC/P, K1ABC/VP9); nullptr when unknown
    // or maritime/aeronautical mobile.
    const DxccEntity* lookup(const QString& call) const;
    // The table bundled in the app (qrc), loaded on first use; empty if the resource is missing.
    static const DxccTable& bundled();
private:
    const DxccEntity* match(const QString& call, bool allow_exact) const;
    std::vector<DxccEntity> entities_;
    QHash<QString, int> exact_, prefix_;
    int longest_ = 0;
};

} // namespace hamclock::dxcluster
