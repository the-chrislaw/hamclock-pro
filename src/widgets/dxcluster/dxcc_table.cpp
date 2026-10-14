#include "dxcc_table.hpp"
#include <QFile>
#include <QRegularExpression>
#include <QStringList>

namespace hamclock::dxcluster {

int DxccTable::parse(const QString& text) {
    entities_.clear(); exact_.clear(); prefix_.clear(); longest_ = 0;
    static const QRegularExpression markup(R"(\([^)]*\)|\[[^\]]*\]|<[^>]*>|\{[^}]*\}|~[^~]*~)");
    bool in_aliases = false;
    DxccEntity pending;
    QStringList exact_pending, prefix_pending;
    for (const QString& raw : text.split('\n')) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) continue;
        if (!in_aliases) {
            const QStringList f = line.split(':');
            if (f.size() < 8) continue;
            bool ok_cq, ok_itu, ok_lat, ok_lon;
            DxccEntity e;
            e.name = f[0].trimmed();
            e.cq_zone = f[1].trimmed().toInt(&ok_cq);
            e.itu_zone = f[2].trimmed().toInt(&ok_itu);
            e.continent = f[3].trimmed();
            e.lat = f[4].trimmed().toDouble(&ok_lat);
            e.lon = -f[5].trimmed().toDouble(&ok_lon);
            e.primary_prefix = f[7].trimmed().remove('*');
            if (!(ok_cq && ok_itu && ok_lat && ok_lon) || e.name.isEmpty() || e.primary_prefix.isEmpty()) continue;
            pending = e;
            exact_pending.clear(); prefix_pending.clear();
            in_aliases = true;
            continue;
        }
        const bool last = line.endsWith(';');
        QString body = line;
        if (last) body.chop(1);
        body.remove(markup);
        for (QString alias : body.split(',', Qt::SkipEmptyParts)) {
            alias = alias.trimmed().toUpper();
            if (alias.startsWith('=')) { alias.remove(0, 1); if (!alias.isEmpty()) exact_pending << alias; }
            else if (!alias.isEmpty()) prefix_pending << alias;
        }
        if (!last) continue;
        // An entity counts only once its alias list is terminated by ';'.
        const int index = int(entities_.size());
        entities_.push_back(pending);
        for (const QString& a : exact_pending) exact_.insert(a, index);
        for (const QString& a : prefix_pending) { prefix_.insert(a, index); longest_ = qMax(longest_, int(a.size())); }
        in_aliases = false;
    }
    return int(entities_.size());
}

const DxccEntity* DxccTable::match(const QString& call, bool allow_exact) const {
    if (allow_exact) {
        const auto it = exact_.constFind(call);
        if (it != exact_.constEnd()) return &entities_[size_t(it.value())];
    }
    for (int n = qMin(int(call.size()), longest_); n > 0; --n) {
        const auto it = prefix_.constFind(call.left(n));
        if (it != prefix_.constEnd()) return &entities_[size_t(it.value())];
    }
    return nullptr;
}

const DxccEntity* DxccTable::lookup(const QString& callsign) const {
    const QString call = callsign.trimmed().toUpper();
    if (call.isEmpty() || entities_.empty()) return nullptr;
    const QStringList parts = call.split('/', Qt::SkipEmptyParts);
    if (parts.isEmpty() || parts.size() > 3) return nullptr;
    if (parts.size() == 1) return match(parts[0], true);
    static const QStringList mobile = {"MM", "AM"};
    for (const QString& p : parts) if (mobile.contains(p)) return nullptr;
    if (exact_.contains(call)) return match(call, true);
    // K1ABC/P, K1ABC/3, K1ABC/QRP: the suffix does not change the entity.
    static const QStringList suffixes = {"P", "M", "A", "B", "QRP", "LH", "LGT", "R", "T"};
    const QString& a = parts[0];
    const QString& b = parts[1];
    auto is_suffix = [&](const QString& s) { return s.size() == 1 || suffixes.contains(s); };
    if (is_suffix(b)) return match(a, true);
    // EA8/K1ABC or K1ABC/EA8: the shorter part is the prefix.
    const QString& prefix = a.size() <= b.size() ? a : b;
    const QString& base = a.size() <= b.size() ? b : a;
    if (const DxccEntity* e = match(prefix, false)) return e;
    return match(base, true);
}

const DxccTable& DxccTable::bundled() {
    static const DxccTable table = [] {
        DxccTable t;
        QFile file(QStringLiteral(":/widgets/dxcluster/data/cty.dat"));
        if (file.open(QIODevice::ReadOnly)) t.parse(QString::fromLatin1(file.readAll()));
        return t;
    }();
    return table;
}

} // namespace hamclock::dxcluster
