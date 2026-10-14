#pragma once
// Read-only system clock synchronisation status. Parsers are pure; the live
// reader runs a local, unprivileged status command (no network, no clock changes)
// and reports "unknown" where the tool is missing (macOS, iPad, containers).
#include <QByteArray>
#include <QString>
#include <optional>

namespace hamclock::platform {

struct ClockSync {
    bool synchronized = false;
    std::optional<double> offset_ms;  // absolute offset from the time source, when known
    std::optional<int> stratum;
};

// `chronyc tracking` -> synchronised if a reference ID is set and the leap status
// is not "Not synchronised"; offset from "Last offset : +0.000012 seconds".
std::optional<ClockSync> parse_chrony_tracking(const QByteArray& text);

// `timedatectl show -p NTPSynchronized` -> "NTPSynchronized=yes|no".
std::optional<ClockSync> parse_timedatectl(const QByteArray& text);

// Tries chronyc, then timedatectl; absent when neither is usable.
std::optional<ClockSync> read_clock_sync();

QString describe(const ClockSync& s);

} // namespace hamclock::platform
