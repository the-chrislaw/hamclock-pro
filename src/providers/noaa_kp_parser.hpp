#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hamclock {

// Original offline decoder for the current NOAA estimated-Kp object schema.
// Bounds are replacement safety policies, not NOAA service guarantees.
constexpr std::size_t kNoaaKpMaximumBytes = 131072;
constexpr std::size_t kNoaaKpMaximumRows = 1024;
constexpr std::size_t kNoaaKpMaximumDepth = 8;
constexpr std::size_t kNoaaKpMaximumMembers = 32;
constexpr std::size_t kNoaaKpMaximumKeyBytes = 128;
constexpr std::size_t kNoaaKpMaximumNumberBytes = 128;

struct NoaaKpRawRow {
    // Exactly yyyy-mm-ddTHH:MM:SS with a valid Gregorian civil date (years
    // 0001..9999). Retained unchanged; NO UTC/local conversion or live-age claim.
    std::string raw_time_tag;
    // null is explicit missing; zero remains a supplied value. Finite 0..9.
    std::optional<double> kp;
    // Required keys, nullable nonnegative lexical integers <= INT32_MAX;
    // fractional/exponent token spellings rejected even if numerically integral.
    std::optional<std::int32_t> a_running;
    std::optional<std::int32_t> station_count;
    // Unknown keys are tolerated within bounds but not interpreted/preserved.
    std::size_t ignored_field_count = 0;
};
struct NoaaKpRawDocument {
    // Caller-supplied descriptive input label: 1..96 printable ASCII bytes;
    // never interpreted as an address, command or source-rights verification.
    std::string provenance_label;
    std::vector<NoaaKpRawRow> rows; // Input order retained; duplicates not deduped.
};
enum class NoaaKpParseError {
    None, InvalidProvenance, EmptyInput, TooManyBytes, InvalidJson,
    TooDeep, TooManyMembers, InvalidKey, DuplicateKey,
    WrongRoot, TooManyRows, InvalidRow, MissingField, InvalidTimestamp,
    InvalidNumber
};
struct NoaaKpParseResult {
    NoaaKpParseError error;
    // All-or-nothing owned immutable result. Null on any parsing rejection;
    // a successful empty array has an owned document with zero rows.
    std::shared_ptr<const NoaaKpRawDocument> document;
};

// QtCore JSON only, with bounded original lexical preflight before Qt parsing:
// root container depth is 1; maximum members per object and decoded UTF8 key
// bytes above; numeric tokens <=128 bytes. Duplicates rejected in EVERY object,
// including Unicode/escape-equivalent keys. Root must be object-row array with
// required time_tag/Kp/a_running/station_count fields. Numeric strings/bools,
// nonfinite/unrepresentable JSON numbers and legacy header-row arrays reject.
// Schema-field validation never treats missing/null readings as synthetic zero.
// Unknown fields may be arbitrary valid JSON under preflight bounds; no schema
// interpretation, source switching, observation interval, quality certification,
// units conversion, data rights, polling, persistence or UI supplied.
// Accepts no BOM; malformed UTF8 and unpaired escaped UTF16 surrogates reject.
// Every numeric token (including unknown fields) must convert to finite double;
// nonzero mantissas underflowing to zero reject rather than fabricate zero.
// Exact decimal precision beyond double is not preserved for Kp/unknown values.
// Row integer metadata additionally requires lexical integer token spelling.
// Allocation exceptions
// propagate to caller; no caller state is mutated, no partial document escapes.
NoaaKpParseResult parse_noaa_kp(std::string_view bytes,
                              std::string_view provenance_label);

} // namespace hamclock
