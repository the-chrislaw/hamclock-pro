#include "../src/app/de_clock_configuration.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<hamclock::DeClockConfigurationState>);
static_assert(!std::is_copy_assignable_v<hamclock::DeClockConfigurationState>);
static_assert(!std::is_move_constructible_v<hamclock::DeClockConfigurationState>);
static_assert(!std::is_move_assignable_v<hamclock::DeClockConfigurationState>);
static_assert(std::is_copy_constructible_v<hamclock::DeClockConfigurationSnapshot>);
static_assert(std::is_copy_assignable_v<hamclock::DeClockConfigurationSnapshot>);

namespace allocation {
bool deny = false;
std::size_t attempts = 0;
}
void* operator new(std::size_t bytes) {
    ++allocation::attempts;
    if (allocation::deny) throw std::bad_alloc();
    if (void* memory = std::malloc(bytes == 0 ? 1 : bytes)) return memory;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

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
template<typename Action>
void rejected(hamclock::DeClockConfigurationState& state, Action action, const char* label) {
    const auto before = state.snapshot();
    bool threw = false;
    try { action(); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, label);
    check(same(before, state.snapshot()), "invalid rejection preserves every snapshot field including coordinate bits");
}
template<typename Action>
void invalid_construction(Action action, const char* label) {
    bool threw = false;
    try { action(); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, label);
}
} // namespace
int main() {
    using namespace hamclock;
    const DeClockConfiguration automatic{DeClockMode::Automatic, std::nullopt};
    const DeClockOffsetPolicy policy{{-3}, {5}}; // explicit test policy, not original limit
    DeClockConfigurationState state(automatic, policy, {0, 180});
    check(state.snapshot().configuration_revision == 1 && state.snapshot().location_revision == 1 &&
          state.snapshot().location.longitude == -180 && !state.snapshot().configuration.manual_offset,
          "initial explicit automatic desire and canonical coordinate retain no resolution payload");
    for (std::int64_t offset : {-3, 0, 5}) {
        auto changed = state.replace({DeClockMode::ManualOffset, UtcOffsetSeconds{offset}});
        check(changed.result == DeClockConfigurationResult::Applied &&
              changed.snapshot.configuration.manual_offset->value == offset && changed.snapshot.location_revision == 1,
              "inclusive explicit manual bounds accept negative zero positive values without location change");
        const auto before = state.snapshot();
        check(state.replace({DeClockMode::ManualOffset, UtcOffsetSeconds{offset}}).result == DeClockConfigurationResult::Unchanged &&
              same(before, state.snapshot()), "same validated desire does not consume revisions");
    }
    const auto manual = state.snapshot();
    check(state.set_location({-0.0, 180}).result == DeClockConfigurationResult::Unchanged && same(manual, state.snapshot()),
          "signed zero and canonical plus180 equivalence preserve manual desire and all coordinate bits");
    check(state.set_location({0, -180}).result == DeClockConfigurationResult::Unchanged,
          "minus180 represents same canonical longitude");
    const auto changed = state.set_location({std::nextafter(0.0, 1.0), -180});
    check(changed.result == DeClockConfigurationResult::Applied && changed.snapshot.configuration.mode == DeClockMode::Automatic &&
          !changed.snapshot.configuration.manual_offset && changed.snapshot.configuration_revision == manual.configuration_revision + 1 &&
          changed.snapshot.location_revision == manual.location_revision + 1,
          "next representable coordinate change resets automatic and advances both revisions without epsilon");
    const auto automatic_before = state.snapshot();
    state.set_location({1, 0});
    check(state.snapshot().configuration_revision == automatic_before.configuration_revision + 1 &&
          state.snapshot().location_revision == automatic_before.location_revision + 1,
          "changed location while already automatic still invalidates prior resolution revisions");
    state.replace({DeClockMode::ManualOffset, UtcOffsetSeconds{1}});
    for (int malformed : {std::numeric_limits<int>::min(), -1, 2, 255, std::numeric_limits<int>::max()}) {
        const DeClockConfiguration invalid{static_cast<DeClockMode>(malformed), std::nullopt};
        rejected(state, [&] { state.replace(invalid); }, "representable unknown fixed-int mode rejected without undefined enum cast");
        invalid_construction([&] { DeClockConfigurationState bad(invalid, policy, {0,0}); }, "constructor rejects unknown desired mode");
    }
    for (const DeClockConfiguration& invalid : {
             DeClockConfiguration{DeClockMode::Automatic, UtcOffsetSeconds{0}},
             DeClockConfiguration{DeClockMode::ManualOffset, std::nullopt},
             DeClockConfiguration{DeClockMode::ManualOffset, UtcOffsetSeconds{-4}},
             DeClockConfiguration{DeClockMode::ManualOffset, UtcOffsetSeconds{6}},
             DeClockConfiguration{DeClockMode::ManualOffset, UtcOffsetSeconds{std::numeric_limits<std::int64_t>::min()}},
             DeClockConfiguration{DeClockMode::ManualOffset, UtcOffsetSeconds{std::numeric_limits<std::int64_t>::max()}}}) {
        rejected(state, [&] { state.replace(invalid); }, "invalid payload or outside-caller-bound offset rejected atomically");
    }
    for (const auto& configuration : {automatic, DeClockConfiguration{DeClockMode::ManualOffset, UtcOffsetSeconds{0}}}) {
        invalid_construction([&] { DeClockConfigurationState bad(configuration, {{1},{-1}}, {0,0}); },
                             "reversed policy rejected even for automatic desire");
    }
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    for (const Location invalid : {Location{nan,0}, Location{0,nan}, Location{infinity,0}, Location{0,infinity},
                                  Location{-infinity,0}, Location{0,-infinity}, Location{90.1,0}, Location{-90.1,0},
                                  Location{0,180.1}, Location{0,-180.1}, Location{std::nextafter(90.0,infinity),0},
                                  Location{0,std::nextafter(180.0,infinity)}}) {
        rejected(state, [&] { state.set_location(invalid); }, "nonfinite and just outside coordinate range rejects atomically");
        invalid_construction([&] { DeClockConfigurationState bad(automatic, policy, invalid); }, "constructor rejects invalid station coordinate");
    }
    for (const Location valid : {Location{90,180}, Location{-90,-180}, Location{0,0}, Location{0,std::nextafter(180.0,0.0)}}) {
        state.set_location(valid);
        check(valid_location(state.snapshot().location), "inclusive finite coordinate corners and representable neighbors valid");
    }
    DeClockConfigurationState wide({DeClockMode::ManualOffset, UtcOffsetSeconds{std::numeric_limits<std::int64_t>::min()}},
                                  {{std::numeric_limits<std::int64_t>::min()}, {std::numeric_limits<std::int64_t>::max()}}, {0,0});
    wide.replace({DeClockMode::ManualOffset, UtcOffsetSeconds{std::numeric_limits<std::int64_t>::max()}});
    check(wide.snapshot().configuration.manual_offset->value == std::numeric_limits<std::int64_t>::max(),
          "full signed offset caller policy has no hidden arithmetic or civil range claim");
    DeClockConfigurationState single({DeClockMode::ManualOffset, UtcOffsetSeconds{7}}, {{7},{7}}, {0,0});
    check(single.snapshot().configuration.manual_offset->value == 7, "single-point caller policy valid without invented zero inclusion");
    rejected(single, [&] { single.replace({DeClockMode::ManualOffset, UtcOffsetSeconds{0}}); }, "single-point bounds enforced");
    auto owned = single.snapshot();
    owned.offset_policy.minimum.value = -999;
    owned.location.longitude = 123;
    owned.configuration_revision = 0;
    check(single.snapshot().offset_policy.minimum.value == 7 && single.snapshot().location.longitude == 0 &&
          single.snapshot().configuration_revision == 1, "owned snapshot mutations cannot replace stable session bounds or revisions");
    check(next_de_clock_revision(0) == 1 && next_de_clock_revision(std::numeric_limits<std::uint64_t>::max() - 1) ==
          std::numeric_limits<std::uint64_t>::max(), "revision helper supports full uint64 range");
    bool exhausted = false;
    try { next_de_clock_revision(std::numeric_limits<std::uint64_t>::max()); }
    catch (const std::overflow_error&) { exhausted = true; }
    check(exhausted, "checked helper rejects revision wrap without session restore backdoor");
    allocation::attempts = 0;
    allocation::deny = true;
    bool heap_free = true;
    try {
        DeClockConfigurationState no_heap(automatic, policy, {0,0});
        no_heap.snapshot();
        no_heap.replace({DeClockMode::ManualOffset, UtcOffsetSeconds{2}});
        no_heap.replace({DeClockMode::ManualOffset, UtcOffsetSeconds{2}});
        no_heap.set_location({1,2});
        no_heap.set_location({1,2});
        no_heap.replace(automatic);
    } catch (const std::bad_alloc&) { heap_free = false; }
    allocation::deny = false;
    check(heap_free && allocation::attempts == 0, "constructor snapshot changed and unchanged successful operations allocate zero heap storage");
    {
        // Zone id desire: Automatic only, bounded shape, allocation-free value.
        DeClockConfigurationState zoned(make_de_clock_zone_configuration("America/New_York"), policy, {0, 0});
        check(de_clock_zone_id(zoned.snapshot().configuration) == "America/New_York" &&
              zoned.snapshot().configuration.mode == DeClockMode::Automatic,
              "Automatic configuration carries a validated IANA zone id");
        check(zoned.replace(make_de_clock_zone_configuration("America/New_York")).result == DeClockConfigurationResult::Unchanged,
              "same zone id is Unchanged");
        check(zoned.replace(make_de_clock_zone_configuration("Europe/London")).result == DeClockConfigurationResult::Applied &&
              zoned.snapshot().configuration_revision == 2, "different zone id is Applied and bumps revision");
        check(zoned.replace({DeClockMode::Automatic, std::nullopt}).result == DeClockConfigurationResult::Applied,
              "clearing the zone id is a change");
        rejected(zoned, [&] { DeClockConfiguration c{DeClockMode::ManualOffset, UtcOffsetSeconds{0}};
                              c.zone_id[0] = 'X'; zoned.replace(c); }, "manual offset must not retain a zone id");
        rejected(zoned, [&] { make_de_clock_zone_configuration(""); }, "empty zone id rejected by builder");
        rejected(zoned, [&] { make_de_clock_zone_configuration("Bad Zone"); }, "space in zone id rejected");
        rejected(zoned, [&] { make_de_clock_zone_configuration(std::string(kDeClockZoneIdBytes, 'A')); },
                 "zone id without room for NUL rejected");
        rejected(zoned, [&] { DeClockConfiguration c{DeClockMode::Automatic, std::nullopt};
                              c.zone_id[0] = 'A'; c.zone_id[1] = 0; c.zone_id[2] = 'B'; zoned.replace(c); },
                 "non-canonical bytes after the terminator rejected");
    }
    std::cout << checks << " DE configuration checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
