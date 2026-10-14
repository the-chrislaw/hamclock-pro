#pragma once
// Watch list: callsigns and prefixes to highlight in spot panes.
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <functional>

namespace hamclock::watchlist {

// Entries are uppercase callsigns ("K1ABC"), prefixes ending in '*' ("VP8*"), or
// "*" patterns with '?' for one character. Portable suffixes (/P, /QRP) and
// prefixes (EA8/) of the spotted call are ignored when matching.
// "@VP8" watches a whole DXCC entity by its primary prefix (as in cty.dat); it needs
// an entity resolver (see matches). Invalid entries (empty, too long, characters outside A-Z 0-9 / * ?) are dropped.
namespace detail {
inline constexpr int kMaxEntries = 200;
inline constexpr int kMaxLength = 16;

// "EA8/K1ABC/P" -> candidates {"EA8/K1ABC/P", "K1ABC/P", "EA8", "K1ABC", ...}: every
// slash-separated part is tried, so both the home call and the operating prefix match.
inline QStringList candidates(const QString& call) {
    QStringList out{call};
    for (const auto& part : call.split('/', Qt::SkipEmptyParts)) out << part;
    return out;
}

inline bool glob(const QString& pattern, const QString& text) {
    const QRegularExpression re(QRegularExpression::anchoredPattern(
        QRegularExpression::escape(pattern).replace("\\*", ".*").replace("\\?", ".")));
    return re.match(text).hasMatch();
}
} // namespace detail

// sanitize: uppercases, dedupes and drops invalid entries; matches: see above.
inline QStringList sanitize(const QStringList& entries) {
    static const QRegularExpression valid("^@?[A-Z0-9/*?]+$");
    QStringList out;
    for (const auto& e : entries) {
        const QString u = e.trimmed().toUpper();
        if (u.isEmpty() || u.size() > detail::kMaxLength || !valid.match(u).hasMatch() || out.contains(u)) continue;
        out << u;
        if (out.size() >= detail::kMaxEntries) break;
    }
    return out;
}

// entity_of maps a call to its DXCC primary prefix (empty when unknown); without it "@" entries never match.
inline bool matches(const QStringList& entries, const QString& call,
                    const std::function<QString(const QString&)>& entity_of = {}) {
    const QString c = call.trimmed().toUpper();
    if (c.isEmpty()) return false;
    const auto cands = detail::candidates(c);
    QString entity;
    bool entity_done = false;
    for (const auto& e : entries) {
        if (e.startsWith('@')) {
            if (!entity_of) continue;
            if (!entity_done) { entity = entity_of(c).toUpper(); entity_done = true; }
            if (!entity.isEmpty() && e.mid(1) == entity) return true;
            continue;
        }
        for (const auto& cand : cands)
            if (detail::glob(e, cand)) return true;
    }
    return false;
}

} // namespace hamclock::watchlist
