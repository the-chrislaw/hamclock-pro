#include "../src/qt/de_clock_display_snapshot.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
int checks = 0, failures = 0;
void check(bool okay, const char* label) {
    ++checks;
    if (!okay) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool same(const hamclock::DeClockConfigurationSnapshot& a, const hamclock::DeClockConfigurationSnapshot& b) {
    return a.configuration.mode == b.configuration.mode &&
        a.configuration.manual_offset.has_value() == b.configuration.manual_offset.has_value() &&
        (!a.configuration.manual_offset || a.configuration.manual_offset->value == b.configuration.manual_offset->value) &&
        a.offset_policy.minimum.value == b.offset_policy.minimum.value &&
        a.offset_policy.maximum.value == b.offset_policy.maximum.value &&
        std::memcmp(&a.location.latitude, &b.location.latitude, sizeof(double)) == 0 &&
        std::memcmp(&a.location.longitude, &b.location.longitude, sizeof(double)) == 0 &&
        a.configuration_revision == b.configuration_revision && a.location_revision == b.location_revision;
}
bool same(const std::optional<hamclock::UtcSeconds>& a, const std::optional<hamclock::UtcSeconds>& b) {
    return a.has_value() == b.has_value() && (!a || a->value == b->value);
}
bool same(const std::optional<hamclock::BigClockCivilTime>& a, const std::optional<hamclock::BigClockCivilTime>& b) {
    return a.has_value() == b.has_value() && (!a || (a->year == b->year && a->month == b->month &&
        a->day == b->day && a->hour == b->hour && a->minute == b->minute && a->second == b->second));
}
void invariants(const hamclock::DeClockDisplaySnapshot& output,
                const hamclock::DeClockConfigurationSnapshot& original,
                std::optional<hamclock::UtcSeconds> source, hamclock::SyncStatus sync) {
    using namespace hamclock;
    check(same(output.configuration, original), "full desired settings policy location and revisions retained coherently");
    check(same(output.actual_source, source) && same(output.utc.source, source) && same(output.de.actual_source, source),
          "both conversions and publication provenance refer to exact same actual source");
    check(output.de.desired_mode == original.configuration.mode &&
          output.de.manual_offset.has_value() == original.configuration.manual_offset.has_value() &&
          (!output.de.manual_offset || output.de.manual_offset->value == original.configuration.manual_offset->value),
          "manual provenance reflects same immutable desired configuration");
    check(output.digital.actual.utc.has_value() == (output.utc.status == BigClockUtcCivilStatus::Ready) &&
          output.digital.actual.de.has_value() == (output.de.status == DeClockManualCivilStatus::Ready) &&
          same(output.digital.actual.utc, output.utc.civil) && same(output.digital.actual.de, output.de.civil),
          "each civil field is present only for corresponding Ready conversion");
    check(!output.digital.preview && output.digital.synchronization == sync,
          "actual basis and explicit synchronization preserved without preview or accuracy inference");
}
} // namespace

int main() {
    using namespace hamclock;
    const auto lowest = std::numeric_limits<std::int64_t>::min();
    const auto highest = std::numeric_limits<std::int64_t>::max();
    const DeClockOffsetPolicy policy{{lowest},{highest}};
    DeClockConfigurationState owner({DeClockMode::Automatic,std::nullopt}, policy, {0,-180});
    const auto automatic = owner.snapshot();
    owner.replace({DeClockMode::ManualOffset,UtcOffsetSeconds{20700}});
    const auto manual = owner.snapshot();
    for (auto sync : {SyncStatus::Unknown, SyncStatus::Synchronized, SyncStatus::Unsynchronized}) {
        for (const auto& config : {automatic, manual}) {
            for (auto source : {std::optional<UtcSeconds>{}, std::optional<UtcSeconds>{UtcSeconds{0}},
                                std::optional<UtcSeconds>{UtcSeconds{-1}}, std::optional<UtcSeconds>{UtcSeconds{highest}}}) {
                const auto before = config;
                auto output = make_de_clock_display_snapshot(config, source, sync);
                invariants(output, before, source, sync);
                check(same(config, before), "successful bridge leaves caller snapshot unchanged");
                if (source && source->value == 0 && config.configuration.mode == DeClockMode::ManualOffset) {
                    check(output.digital.actual.utc->year == 1970 && output.digital.actual.utc->hour == 0 &&
                          output.digital.actual.de->hour == 5 && output.digital.actual.de->minute == 45 &&
                          output.de.shifted_calendar->value == 20700 && output.actual_source->value == 0,
                          "coherent epoch zero quarter-hour manual civil remains separate from actual UTC");
                }
                if (config.configuration.mode == DeClockMode::Automatic && source) {
                    check(output.de.status == DeClockManualCivilStatus::AutomaticUnresolved && !output.digital.actual.de,
                          "automatic desire cannot silently fallback to UTC DE civil");
                }
            }
        }
    }
    const struct { std::int64_t source, offset; DeClockManualCivilStatus status; } fixtures[] = {
        {0,0,DeClockManualCivilStatus::Ready}, {0,-1,DeClockManualCivilStatus::Ready},
        {951782399,1,DeClockManualCivilStatus::Ready}, {4107542399,1,DeClockManualCivilStatus::Ready},
        {1,highest,DeClockManualCivilStatus::OffsetOverflow}, {-1,lowest,DeClockManualCivilStatus::OffsetOverflow},
        {0,highest,DeClockManualCivilStatus::DisplayOutOfRange}, {0,lowest,DeClockManualCivilStatus::DisplayOutOfRange},
        {kBigClockUtcCivilMinimumSeconds,-1,DeClockManualCivilStatus::DisplayOutOfRange},
        {kBigClockUtcCivilMaximumSeconds,1,DeClockManualCivilStatus::DisplayOutOfRange},
        {lowest,0,DeClockManualCivilStatus::SourceOutOfRange}, {highest,0,DeClockManualCivilStatus::SourceOutOfRange}
    };
    for (const auto& fixture : fixtures) {
        auto config = manual;
        config.configuration.manual_offset = UtcOffsetSeconds{fixture.offset};
        auto output = make_de_clock_display_snapshot(config, UtcSeconds{fixture.source}, SyncStatus::Unknown);
        check(output.de.status == fixture.status, "bridge preserves guarded manual classification instead of masking status");
        invariants(output, config, UtcSeconds{fixture.source}, SyncStatus::Unknown);
    }
    for (const auto& source : {std::optional<UtcSeconds>{}, std::optional<UtcSeconds>{UtcSeconds{0}}}) {
        for (const auto& config : {automatic, manual}) {
            for (int bad_sync : {std::numeric_limits<int>::min(), -1, 3, 255, std::numeric_limits<int>::max()}) {
                bool threw = false;
                try { make_de_clock_display_snapshot(config, source, static_cast<SyncStatus>(bad_sync)); }
                catch (const std::invalid_argument&) { threw = true; }
                check(threw, "malformed fixed-int synchronization rejected even for missing source or automatic");
            }
            for (int scenario = 0; scenario < 13; ++scenario) {
                auto invalid = config;
                switch (scenario) {
                case 0: invalid.offset_policy = {{1},{-1}}; break;
                case 1: invalid.configuration.mode = static_cast<DeClockMode>(255); break;
                case 2: invalid.configuration = {DeClockMode::Automatic,UtcOffsetSeconds{0}}; break;
                case 3: invalid.configuration = {DeClockMode::ManualOffset,std::nullopt}; break;
                case 4: invalid.configuration_revision = 0; break;
                case 5: invalid.location_revision = 0; break;
                case 6: invalid.configuration_revision = 1; invalid.location_revision = 2; break;
                case 7: invalid.location.latitude = std::numeric_limits<double>::infinity(); break;
                case 8: invalid.location.longitude = std::numeric_limits<double>::quiet_NaN(); break;
                case 9: invalid.location.longitude = 180; break;
                case 10: invalid.location.latitude = std::nextafter(90.0,100.0); break;
                case 11: invalid.location.longitude = std::nextafter(-180.0,-200.0); break;
                case 12: invalid.configuration = {DeClockMode::ManualOffset,UtcOffsetSeconds{1}}; invalid.offset_policy = {{0},{0}}; break;
                }
                const auto before = invalid;
                bool threw = false;
                try { make_de_clock_display_snapshot(invalid, source, SyncStatus::Unknown); }
                catch (const std::invalid_argument&) { threw = true; }
                check(threw, "malformed full snapshot schema rejected before unavailable conversion branch");
                check(same(before, invalid), "invalid bridge input preserves all caller fields and coordinate bits");
            }
        }
    }
    auto canonical = automatic;
    canonical.location = {-0.0,-180};
    canonical.configuration_revision = canonical.location_revision = std::numeric_limits<std::uint64_t>::max();
    invariants(make_de_clock_display_snapshot(canonical, UtcSeconds{0}, SyncStatus::Unknown), canonical, UtcSeconds{0}, SyncStatus::Unknown);
    auto owned = make_de_clock_display_snapshot(manual, UtcSeconds{0}, SyncStatus::Unknown);
    owned.configuration.configuration.manual_offset->value = 99;
    owned.actual_source->value = 99;
    owned.de.shifted_calendar->value = 99;
    owned.digital.actual.de->hour = 23;
    auto original_again = make_de_clock_display_snapshot(manual, UtcSeconds{0}, SyncStatus::Unknown);
    check(original_again.actual_source->value == 0 && original_again.configuration.configuration.manual_offset->value == 20700 &&
          original_again.digital.actual.de->hour == 5, "owned publication mutations cannot alias settings source or future conversions");
    const BigClockPreferences utc_pref{BigClockFace::Digital,BigClockZone::Utc,BigClockHourFormat::TwentyFour,
                                     true,true,false,false,false,false};
    auto de_pref = utc_pref;
    de_pref.zone = BigClockZone::De;
    const auto formatted_utc = present_big_clock_digital(utc_pref, original_again.digital);
    const auto formatted_de = present_big_clock_digital(de_pref, original_again.digital);
    check(std::string(formatted_utc.time_text.data()) == "00:00:00" &&
          std::string(formatted_de.time_text.data()) == "05:45:00" &&
          formatted_utc.synchronization == SyncStatus::Unknown && formatted_de.synchronization == SyncStatus::Unknown,
          "frozen formatter consumes coherent UTC and DE values without rewriting unknown synchronization");
    const auto auto_display = make_de_clock_display_snapshot(automatic, UtcSeconds{0}, SyncStatus::Synchronized);
    check(present_big_clock_digital(de_pref, auto_display.digital).status == BigClockDigitalStatus::MissingSelectedTime &&
          present_big_clock_digital(utc_pref, auto_display.digital).status == BigClockDigitalStatus::Ready,
          "selected missing automatic DE never borrows UTC despite synchronized source evidence");
    {
        // 2024-07-01 12:00:00 UTC (EDT, UTC-4) -> 08:00 New York; 2024-01-15 12:00 UTC (EST) -> 07:00.
        DeClockConfigurationState zoned(make_de_clock_zone_configuration("America/New_York"), policy, {40.7, -74.0});
        const auto summer = make_de_clock_display_snapshot(zoned.snapshot(), UtcSeconds{1719835200}, SyncStatus::Synchronized);
        check(summer.de.status == DeClockManualCivilStatus::Ready && summer.de.civil && summer.de.civil->hour == 8 &&
              summer.digital.actual.de && summer.digital.actual.de->hour == 8, "zone Automatic DE shows EDT in July");
        const auto winter = make_de_clock_display_snapshot(zoned.snapshot(), UtcSeconds{1705320000}, SyncStatus::Synchronized);
        check(winter.de.civil && winter.de.civil->hour == 7, "zone Automatic DE shows EST in January");
        check(!summer.de.manual_offset && summer.de.desired_mode == DeClockMode::Automatic, "zone result is not a manual offset");
        const auto none = make_de_clock_display_snapshot(zoned.snapshot(), std::nullopt, SyncStatus::Unknown);
        check(none.de.status == DeClockManualCivilStatus::MissingSource && !none.digital.actual.de, "missing source stays unavailable");
        DeClockConfigurationState unknown(make_de_clock_zone_configuration("Mars/Olympus"), policy, {0, 0});
        const auto bad = make_de_clock_display_snapshot(unknown.snapshot(), UtcSeconds{0}, SyncStatus::Synchronized);
        check(bad.de.status == DeClockManualCivilStatus::AutomaticUnresolved && !bad.digital.actual.de,
              "unknown zone never falls back to UTC or host zone");
    }
    std::cout << checks << " DE display snapshot checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
