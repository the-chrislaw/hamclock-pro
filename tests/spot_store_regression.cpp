#include "spots/spot_store.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <functional>

using namespace hamclock::spots;
namespace {
int checks = 0;
void check(bool yes, const char* label) {
    ++checks;
    if (!yes) throw std::runtime_error(label);
}
bool same(const SpotEvent& a, const SpotEvent& b) {
    return a.key == b.key && a.frequency.value == b.frequency.value && a.callsign == b.callsign
        && a.spotter_callsign == b.spotter_callsign && a.reported.unix_seconds == b.reported.unix_seconds
        && a.reported.quality == b.reported.quality && a.location.has_value() == b.location.has_value()
        && (!a.location || (a.location->latitude == b.location->latitude && a.location->longitude == b.location->longitude))
        && a.location_provenance == b.location_provenance;
}
bool same(const std::vector<SpotEvent>& a, const std::vector<SpotEvent>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) if (!same(a[i], b[i])) return false;
    return true;
}
SpotEvent event(std::string id = "e1", std::string source = "fixture") {
    return {{std::move(source), std::move(id)}, {14'074'000}, "n0call/p", std::string("k1abc"),
        {{100}, TimeQuality::Reported}, hamclock::Location{0, 180}, "synthetic:explicit-coordinate"};
}
void rejected(const std::function<void()>& fn) {
    bool threw = false;
    try { fn(); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "expected invalid_argument");
}
void run() {
    rejected([] { SpotStore x(0); });
    rejected([] { SpotStore x(10001); });
    SpotStore maximum(10000);
    check(maximum.capacity() == 10000 && maximum.size() == 0, "capacity boundary");
    SpotStore store(2);
    auto first = event();
    auto result = store.upsert(first);
    check(result.kind == UpsertKind::Inserted && !result.evicted, "first insert");
    check(first.callsign == "n0call/p", "normalization preserves input");
    auto saved = store.snapshot();
    check(saved[0].callsign == "N0CALL/P" && *saved[0].spotter_callsign == "K1ABC", "ASCII normalization");
    check(saved[0].location->longitude == 180 && saved[0].frequency.value == 14'074'000, "units and endpoint retained");
    saved[0].callsign = "OTHER";
    check(store.snapshot()[0].callsign == "N0CALL/P", "snapshot independent");
    store.upsert(event("e2"));
    auto updated = first;
    updated.frequency.value = 7'074'000;
    updated.reported.unix_seconds = 150;
    updated.location.reset(); updated.location_provenance.clear(); updated.spotter_callsign.reset();
    result = store.upsert(updated);
    check(result.kind == UpsertKind::Updated && !result.evicted && store.size() == 2, "dedup update");
    check(same(store.snapshot()[0], normalize_event(updated)), "all update metadata replaced");
    result = store.upsert(event("e3"));
    check(result.evicted && *result.evicted == first.key, "update retains oldest arrival eviction");
    check(store.snapshot()[0].key.event_identity == "e2" && store.snapshot()[1].key.event_identity == "e3", "arrival deterministic");
    SpotStore identities(4);
    identities.upsert(first); identities.upsert(event("e1", "other")); identities.upsert(event("e2"));
    check(identities.size() == 3, "same call frequency does not cross-key dedup");
    identities.upsert(event("e1", "Fixture"));
    check(identities.size() == 4, "source identity case preserved");

    auto invalid = [&](const std::function<void(SpotEvent&)>& edit) {
        const auto before = store.snapshot(); auto bad = event("e2"); edit(bad);
        rejected([&] { store.upsert(bad); });
        check(same(before, store.snapshot()), "invalid input preserves full snapshot");
    };
    invalid([](auto& e) { e.frequency.value = 0; });
    invalid([](auto& e) { e.frequency.value = max_frequency_hz + 1; });
    invalid([](auto& e) { e.frequency.value = std::numeric_limits<std::uint64_t>::max(); });
    invalid([](auto& e) { e.callsign.clear(); });
    invalid([](auto& e) { e.callsign = std::string(33, 'A'); });
    invalid([](auto& e) { e.callsign = "///"; });
    invalid([](auto& e) { e.callsign = "<script>"; });
    invalid([](auto& e) { e.callsign = std::string("A\0B", 3); });
    invalid([](auto& e) { e.callsign = "A\xff"; });
    invalid([](auto& e) { e.spotter_callsign = "bad call"; });
    invalid([](auto& e) { e.key.source.clear(); });
    invalid([](auto& e) { e.key.source = std::string(65, 'a'); });
    invalid([](auto& e) { e.key.event_identity = std::string(129, 'a'); });
    invalid([](auto& e) { e.key.event_identity = "line\ncommand"; });
    invalid([](auto& e) { e.location = hamclock::Location{91, 0}; });
    invalid([](auto& e) { e.location = hamclock::Location{0, -181}; });
    invalid([](auto& e) { e.location = hamclock::Location{std::numeric_limits<double>::quiet_NaN(), 0}; });
    invalid([](auto& e) { e.location = hamclock::Location{0, std::numeric_limits<double>::infinity()}; });
    invalid([](auto& e) { e.location_provenance.clear(); });
    invalid([](auto& e) { e.location.reset(); });
    invalid([](auto& e) { e.location_provenance = std::string(257, 'a'); });
    invalid([](auto& e) { e.location_provenance = "escape\x1b[0m"; });
    invalid([](auto& e) { e.reported.unix_seconds.reset(); });
    invalid([](auto& e) { e.reported.quality = static_cast<TimeQuality>(99); });
    auto limits = event("e4", std::string(64, 'a'));
    limits.key.event_identity = std::string(128, 'a'); limits.callsign = std::string(32, 'a');
    limits.location_provenance = std::string(256, ' '); limits.frequency.value = max_frequency_hz;
    check(normalize_event(limits).callsign == std::string(32, 'A'), "exact text/frequency bounds");
    limits.frequency.value = 1; limits.location = hamclock::Location{-90, -180};
    check(normalize_event(limits).frequency.value == 1, "minimum and negative endpoint bounds");
    limits.location = hamclock::Location{90, 180};
    check(normalize_event(limits).location->latitude == 90, "north pole bound");
    limits.reported = {std::nullopt, TimeQuality::Unknown};
    check(!normalize_event(limits).reported.unix_seconds, "unknown timestamp not fabricated");

    const AgePolicy policy{60, 2};
    check(age_at({100, TimeQuality::Reported}, 160, policy).status == AgeStatus::Fresh, "exact stale threshold");
    check(age_at({100, TimeQuality::Reported}, 161, policy).status == AgeStatus::Stale, "stale threshold crossed");
    check(age_at({100, TimeQuality::Verified}, 100, {0, 0}).seconds == 0, "zero policy exact now");
    check(age_at({100, TimeQuality::Reported}, 101, {0, 0}).status == AgeStatus::Stale, "zero stale policy");
    check(age_at({100, TimeQuality::Reported}, 99, {0, 0}).status == AgeStatus::Future, "zero future policy");
    check(age_at({102, TimeQuality::Reported}, 100, policy).seconds == 0, "exact future tolerance");
    check(age_at({103, TimeQuality::Reported}, 100, policy).status == AgeStatus::Future, "future threshold crossed");
    check(age_at({103, TimeQuality::Reported}, 100, policy).seconds == 3, "future lead explicit");
    check(age_at({100, TimeQuality::Unknown}, 200, policy).status == AgeStatus::Unknown, "untrusted time unknown");
    check(!age_at({std::nullopt, TimeQuality::Unknown}, 200, policy).seconds, "absent age unknown");
    rejected([&] { age_at({std::nullopt, TimeQuality::Reported}, 0, policy); });
    rejected([&] { age_at({std::nullopt, TimeQuality::Verified}, 0, policy); });
    rejected([&] { age_at({0, static_cast<TimeQuality>(99)}, 0, policy); });
    const auto lo = std::numeric_limits<std::int64_t>::min();
    const auto hi = std::numeric_limits<std::int64_t>::max();
    check(age_at({lo, TimeQuality::Verified}, hi, policy).seconds == std::numeric_limits<std::uint64_t>::max(), "extreme elapsed exact");
    check(age_at({hi, TimeQuality::Verified}, lo, policy).seconds == std::numeric_limits<std::uint64_t>::max(), "extreme lead exact");
    check(age_at({86399, TimeQuality::Reported}, 86401, policy).seconds == 2, "UTC midnight age");
    check(age_at({100, TimeQuality::Reported}, 161, policy).status == AgeStatus::Stale
        && age_at({100, TimeQuality::Reported}, 90, policy).status == AgeStatus::Future, "backward clock correction recalculated");
    SpotStore one(1); one.upsert(first);
    check(one.upsert(event("next")).evicted->event_identity == "e1" && one.size() == 1, "capacity one");
    store.clear(); check(store.size() == 0 && store.capacity() == 2, "clear retains capacity");
}
}
int main() {
    try { run(); std::cout << "PASS " << checks << " spot model checks\n"; return 0; }
    catch (const std::exception& e) { std::cerr << "FAIL after " << checks << ": " << e.what() << '\n'; return 1; }
}
