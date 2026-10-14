#pragma once
// APRS-IS packet lines ("CALL>DEST,PATH:payload") reduced to station positions. Handles plain and compressed
// position reports ('!', '=', '/', '@'); everything else (Mic-E, objects, messages, telemetry) is ignored.
#include <QString>
#include <optional>

namespace hamclock::aprs {

struct Station {
    QString call;
    double latitude = 0, longitude = 0;
    QString symbol; // table char + code char, e.g. "/>"
    QString comment;
};

std::optional<Station> parse_packet(const QString& line);

} // namespace hamclock::aprs
