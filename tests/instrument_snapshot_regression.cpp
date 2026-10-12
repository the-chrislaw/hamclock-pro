#include "../src/app/instrument_snapshot.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <functional>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* message) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
void rejects(const std::function<void()>& action, const char* message) {
    try { action(); check(false, message); }
    catch (const std::invalid_argument&) { check(true, message); }
}
bool same_snapshot(const hamclock::InstrumentSnapshot& a, const hamclock::InstrumentSnapshot& b) {
    bool same = a.instrument_id == b.instrument_id &&
        a.provenance.provider_id == b.provenance.provider_id &&
        a.provenance.source_revision == b.provenance.source_revision &&
        a.provenance.attribution == b.provenance.attribution &&
        a.fetched_at.value == b.fetched_at.value && a.retrieved_at.value == b.retrieved_at.value &&
        a.refresh_deadline.value == b.refresh_deadline.value &&
        a.observation.has_value() == b.observation.has_value() && a.forecast.has_value() == b.forecast.has_value();
    if (a.observation && b.observation) {
        same = same && a.observation->observed_at.value == b.observation->observed_at.value &&
            a.observation->value == b.observation->value && a.observation->unit == b.observation->unit &&
            a.observation->quality == b.observation->quality;
    }
    if (a.forecast && b.forecast) {
        same = same && a.forecast->issued_at.value == b.forecast->issued_at.value &&
            a.forecast->valid_at.value == b.forecast->valid_at.value && a.forecast->value == b.forecast->value &&
            a.forecast->unit == b.forecast->unit;
    }
    return same;
}
hamclock::InstrumentSnapshot fixture() {
    using namespace hamclock;
    return {InstrumentId::SolarFlux, {"synthetic-fixture", "fixture-v1", "Original synthetic test data"},
            {100}, {1000}, {2000}, InstrumentReading{{90}, 0.0, InstrumentUnit::Sfu, ReadingQuality::Valid},
            InstrumentForecast{{95}, {200}, 123.0, InstrumentUnit::Sfu}};
}
} // namespace

int main() {
    using namespace hamclock;
    InstrumentSnapshotState state(InstrumentId::SolarFlux);
    check(!state.snapshot(), "initial cache absent, not synthetic zero");
    check(state.phase() == InstrumentPhase::Unavailable, "initial unavailable");
    check(state.evaluate({100}, {1000}, {{10}}).status == InstrumentStatus::Unavailable,
          "initial evaluation unavailable");
    rejects([] { InstrumentSnapshotState bad(static_cast<InstrumentId>(0)); }, "invalid state ID rejected");
    auto sample = fixture();
    state.accept(sample);
    check(state.phase() == InstrumentPhase::Ready, "valid accept ready");
    auto value = state.evaluate({100}, {1999}, {{10}});
    check(value.status == InstrumentStatus::Current && value.observation_usable,
          "inclusive observation age fresh before deadline");
    check(value.observation_age_seconds == 10 && value.forecast_present,
          "observation age separate from future forecast");
    check(state.snapshot()->observation->value == 0.0, "observed zero remains a valid number");
    check(state.evaluate({100}, {2000}, {{10}}).status == InstrumentStatus::Stale,
          "deadline equality stale");
    check(state.evaluate({100}, {2000}, {{10}}).reason == InstrumentStatusReason::RefreshDue,
          "deadline reason explicit");
    check(state.evaluate({101}, {1999}, {{10}}).reason == InstrumentStatusReason::AgeExpired,
          "age after inclusive limit stale");
    check(state.evaluate({89}, {1999}, {{10}}).reason == InstrumentStatusReason::FutureTimestamp,
          "wall-clock backward jump reports future evidence");
    check(!state.evaluate({89}, {1999}, {{10}}).observation_usable, "future reading not usable");
    check(state.evaluate({100}, {999}, {{10}}).reason == InstrumentStatusReason::MonotonicBeforeRetrieval,
          "monotonic rollback rejected in evaluation");
    rejects([&] { state.evaluate({100}, {1000}, {{-1}}); }, "negative policy rejected");
    for (const auto phase : {InstrumentPhase::Loading, InstrumentPhase::Error, InstrumentPhase::Offline}) {
        state.set_phase(phase);
        value = state.evaluate({100}, {1500}, {{10}});
        check(value.observation_usable && value.observation_age_seconds == 10,
              "cache preserved through fetch failure/loading");
        check(value.status == (phase == InstrumentPhase::Offline ? InstrumentStatus::OfflineCached : InstrumentStatus::Stale),
              "fetch state cannot pretend cached reading current");
        check(state.snapshot()->observation->value == 0.0, "cached number preserved");
    }
    value = state.evaluate({1000}, {2500}, {{10}});
    check(value.status == InstrumentStatus::OfflineCached && value.observation_age_seconds == 910,
          "offline old reading keeps explicit age");
    for (const auto phase : {InstrumentPhase::Disabled, InstrumentPhase::Unsupported, InstrumentPhase::Unavailable}) {
        state.set_phase(phase);
        value = state.evaluate({100}, {1500}, {{10}});
        check(!value.observation_usable && !value.forecast_present, "nonenabled state hides data");
        check(state.snapshot().has_value(), "hidden cache retained");
    }
    state.set_phase(InstrumentPhase::Error);
    const auto saved_snapshot = *state.snapshot();
    const auto bad_accept = [&](InstrumentSnapshot bad, const char* message) {
        rejects([&] { state.accept(bad); }, message);
        check(state.phase() == InstrumentPhase::Error &&
              same_snapshot(*state.snapshot(), saved_snapshot),
              "failed replacement preserves phase and cache");
    };
    auto bad = sample; bad.instrument_id = InstrumentId::Moon; bad_accept(bad, "wrong family rejected");
    bad = sample; bad.instrument_id = static_cast<InstrumentId>(65535); bad_accept(bad, "unknown family rejected");
    bad = sample; bad.observation->value = std::numeric_limits<double>::quiet_NaN(); bad_accept(bad, "NaN rejected");
    bad = sample; bad.observation->value = std::numeric_limits<double>::infinity(); bad_accept(bad, "infinity rejected");
    bad = sample; bad.observation->quality = ReadingQuality::Missing; bad_accept(bad, "missing with numeric value rejected");
    bad = sample; bad.observation->value.reset(); bad_accept(bad, "valid without number rejected");
    bad = sample; bad.observation->unit = static_cast<InstrumentUnit>(255); bad_accept(bad, "unknown unit rejected");
    bad = sample; bad.observation->quality = static_cast<ReadingQuality>(255); bad_accept(bad, "unknown quality rejected");
    bad = sample; bad.forecast->value = std::numeric_limits<double>::quiet_NaN(); bad_accept(bad, "NaN forecast rejected");
    bad = sample; bad.forecast->unit = static_cast<InstrumentUnit>(255); bad_accept(bad, "unknown forecast unit rejected");
    bad = sample; bad.forecast->issued_at = {201}; bad_accept(bad, "issue after validity rejected");
    bad = sample; bad.refresh_deadline = {999}; bad_accept(bad, "deadline before retrieval rejected");
    for (const int field : {0, 1, 2}) {
        for (const int kind : {0, 1, 2, 3}) {
            bad = sample;
            std::string* text = field == 0 ? &bad.provenance.provider_id :
                field == 1 ? &bad.provenance.source_revision : &bad.provenance.attribution;
            *text = kind == 0 ? "" : kind == 1 ? std::string(field == 0 ? 65 : field == 1 ? 97 : 257, 'a') :
                kind == 2 ? std::string("embedded\0nul", 12) : std::string(1, '\x80');
            bad_accept(bad, "invalid provenance rejected atomically");
        }
    }
    rejects([&] { state.set_phase(static_cast<InstrumentPhase>(255)); }, "unknown phase rejected");
    check(state.phase() == InstrumentPhase::Error && same_snapshot(*state.snapshot(), saved_snapshot), "invalid phase retains complete previous state");
    for (const auto quality : {ReadingQuality::Missing, ReadingQuality::Invalid}) {
        auto gap = sample; gap.observation->quality = quality; gap.observation->value.reset();
        state.accept(gap);
        value = state.evaluate({100}, {1500}, {{10}});
        check(!value.observation_usable && !value.observation_age_seconds,
              "explicit gap never becomes zero or stale numeric reading");
        check(value.status == (quality == ReadingQuality::Missing ? InstrumentStatus::Unavailable : InstrumentStatus::Error),
              "missing and invalid distinct failure status");
    }
    auto forecast_only = sample; forecast_only.observation.reset();
    state.accept(forecast_only);
    value = state.evaluate({100}, {1500}, {{10}});
    check(value.status == InstrumentStatus::Unavailable && value.forecast_present && !value.observation_usable,
          "forecast-only snapshot cannot claim a current observation");
    for (const auto phase : {InstrumentPhase::Loading, InstrumentPhase::Error, InstrumentPhase::Offline}) {
        state.set_phase(phase);
        value = state.evaluate({100}, {1500}, {{10}});
        check(value.status == (phase == InstrumentPhase::Loading ? InstrumentStatus::Loading :
                              phase == InstrumentPhase::Error ? InstrumentStatus::Error : InstrumentStatus::Unavailable),
              "empty observation has explicit fetch state");
    }
    auto edge = fixture(); edge.fetched_at = {0}; edge.observation->observed_at = {0};
    edge.forecast.reset(); edge.retrieved_at = {std::numeric_limits<std::int64_t>::min()};
    edge.refresh_deadline = {std::numeric_limits<std::int64_t>::max()};
    state.accept(edge);
    check(state.evaluate({0}, {0}, {{0}}).status == InstrumentStatus::Current, "epoch zero fresh with zero maximum age");
    edge.observation->observed_at = {std::numeric_limits<std::int64_t>::min()};
    edge.fetched_at = edge.observation->observed_at; state.accept(edge);
    value = state.evaluate({std::numeric_limits<std::int64_t>::max()}, {0}, {{std::numeric_limits<std::int64_t>::max()}});
    check(value.observation_age_seconds == std::numeric_limits<std::uint64_t>::max(),
          "full unsigned age across signed64 extremes");
    check(value.status == InstrumentStatus::Stale, "extreme old reading remains stale");
    sample = fixture(); sample.fetched_at = {101}; state.accept(sample);
    check(state.evaluate({100}, {1500}, {{10}}).reason == InstrumentStatusReason::FutureTimestamp,
          "future fetch timestamp detected independently of observation");
    sample = fixture(); sample.forecast->issued_at = {101}; state.accept(sample);
    value = state.evaluate({100}, {1500}, {{10}});
    check(value.status == InstrumentStatus::Error && !value.forecast_present,
          "future forecast issue is suspect rather than an accepted forecast");
    sample = fixture(); sample.observation->quality = ReadingQuality::Missing;
    sample.observation->value.reset(); sample.observation->observed_at = {101}; state.accept(sample);
    check(state.evaluate({100}, {1500}, {{10}}).reason == InstrumentStatusReason::FutureTimestamp,
          "future missing-reading timestamp still detected");
    for (const auto phase : {InstrumentPhase::Unavailable, InstrumentPhase::Disabled, InstrumentPhase::Unsupported}) {
        state.set_phase(phase);
        rejects([&] { state.evaluate({100}, {1500}, {{-1}}); }, "hidden state still validates policy");
        rejects([&] { evaluate_instrument_snapshot(std::nullopt, static_cast<InstrumentPhase>(255),
            {100}, {1500}, {{10}}); }, "absent data still validates phase");
    }
    sample = fixture(); sample.provenance.provider_id = std::string(64, 'a');
    sample.provenance.source_revision = std::string(96, 'r'); sample.provenance.attribution = std::string(256, 'c');
    state.accept(sample);
    check(same_snapshot(*state.snapshot(), sample), "maximum provenance bounds accepted");
    sample.provenance.provider_id = "mutated-after-accept"; sample.observation->value = 7.0;
    check(state.snapshot()->provenance.provider_id == std::string(64, 'a') &&
          state.snapshot()->observation->value == 0.0, "accept deep-copies caller-owned strings and readings");
    auto invalid_public = fixture(); invalid_public.provenance.provider_id.clear();
    rejects([&] { evaluate_instrument_snapshot(invalid_public, InstrumentPhase::Disabled, {100}, {1500}, {{10}}); },
            "public hidden evaluator still validates malformed snapshot");
    std::cout << checks << " instrument snapshot checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
