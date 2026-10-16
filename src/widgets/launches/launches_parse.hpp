#pragma once
// Parser for Launch Library 2 upcoming-launch lists
// (ll.thespacedevs.com/2.2.0/launch/upcoming/?mode=list). Pure functions: no
// network, no clock.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <vector>

namespace hamclock::launches {

struct Launch {
    QString name;      // "Falcon 9 Block 5 | USSF-481"
    QString provider;  // "SpaceX"
    QString pad;
    QString location;
    QString status;    // abbreviation: Go, TBC, TBD, Success, Hold ...
    QString mission;
    QString mission_type;  // "Dedicated Rideshare", "Communications" ...
    QDateTime net;     // no-earlier-than time, UTC
    bool precise = false;  // time accurate to the minute or better
};

struct Feed {
    bool ok = false;
    std::vector<Launch> launches;  // soonest first
};

// Rows with no name or an unreadable time are skipped. A body without a
// "results" array yields ok == false.
Feed parse(const QByteArray& body);

// A short hint when the launch may be of interest to amateurs, from the name
// and mission text only: rideshares/cubesat flights often carry amateur radio
// satellites, ISS visits bring ARISS opportunities. It is a hint, never a
// claim about payloads; empty when nothing matches.
QString ham_hint(const Launch& launch);

// "T-2d 3h", "T-5h 12m", "T-12m", "T+0m" once past.
QString countdown_text(const QDateTime& net, const QDateTime& now);

// Launches at or after `now` minus a 2 h grace (so a launch just under way
// is still shown), at most `limit`.
std::vector<Launch> upcoming(const Feed& feed, const QDateTime& now, int limit);

} // namespace hamclock::launches
