// Age of the SWPC values on the Space Wx, HF bands and HF paths panes, through the C ABI the Swift
// panes call. Same limits and wording as the main app (src/widgets/spaceweather/swpc_parse.hpp).
#include "../hcpcore.h"
#include "../swpc.hpp"
#include "test_util.hpp"
#include <string>

using namespace hcptest;

namespace {
constexpr int64_t kMinute = 60 * 1000LL;
constexpr int64_t kHour = 60 * kMinute;

int64_t at(const char* iso) { return hcp::swpc::parse_iso_utc_ms(iso).value_or(-1); }

HcpIndexInput shown(int64_t time_ms) {
    HcpIndexInput in{};
    in.has_value = 1;
    in.time_ms = time_ms;
    return in;
}

HcpIndexInput missing() { return HcpIndexInput{}; }

HcpIndexInput waiting() {
    HcpIndexInput in{};
    in.waiting = 1;
    return in;
}

HcpIndexInput late_feed(int64_t time_ms) {
    HcpIndexInput in = shown(time_ms);
    in.feed_late = 1;
    return in;
}

HcpIndexCheck space(HcpIndexInput kp, HcpIndexInput sfi, HcpIndexInput xray, int64_t now) {
    HcpIndexCheck out{};
    hcp_swpc_check_space_weather(&kp, &sfi, &xray, now, &out);
    return out;
}

HcpIndexCheck hf(HcpIndexInput sfi, HcpIndexInput kp, int64_t now) {
    HcpIndexCheck out{};
    hcp_swpc_check_hf(&sfi, &kp, now, &out);
    return out;
}

bool is_live(const HcpIndexCheck& c) {
    return c.stale == 0 && c.oldest_ms == 0 && !c.note[0] && !c.short_note[0] && !c.tiny_note[0];
}

void limits() {
    using namespace hcp::swpc;
    check(kKpMaxAgeMs == 9 * kHour && kSolarFluxMaxAgeMs == 84 * kHour && kXrayMaxAgeMs == 30 * kMinute, "limits as the main app");
    const int64_t now = at("2026-11-02T12:00:00Z");
    check(!is_old(now - kKpMaxAgeMs, now, kKpMaxAgeMs), "exactly at the limit is not old");
    check(is_old(now - kKpMaxAgeMs - 1000, now, kKpMaxAgeMs), "past the limit is old");
    check(!is_old(0, now, kKpMaxAgeMs), "unknown time is not old");
    check(!is_old(now + kHour, now, kKpMaxAgeMs), "a tag ahead of the clock is not old");
}

void healthy_feeds_stay_live() {
    const int64_t now = at("2026-11-02T01:00:00Z");
    // The real feeds at 2026-11-02 00:58 UTC (#778): Kp 21:00, SFI 20:00, X-ray 4 min old.
    const auto kp = shown(at("2026-11-01T21:00:00Z")), sfi = shown(at("2026-11-01T20:00:00Z")), xray = shown(now - 4 * kMinute);
    check(is_live(space(kp, sfi, xray, now)), "fresh: Space Wx live");
    check(is_live(hf(sfi, kp, now)), "fresh: HF panes live");

    // Just before the next Kp row: the old 6 h TV limit raised a false alarm here.
    const auto six_hours = shown(now - 6 * kHour - 10 * kMinute);
    check(is_live(space(six_hours, sfi, xray, now)), "Kp row late by minutes: Space Wx live");
    check(is_live(hf(sfi, six_hours, now)), "Kp row late by minutes: HF panes live");
    const auto one_missed = shown(now - 9 * kHour);
    check(is_live(space(one_missed, sfi, xray, now)), "one missed Kp row: live");
    check(is_live(space(kp, sfi, shown(now - 30 * kMinute), now)), "X-ray at its limit: live");
}

void friday_sfi_on_monday() {
    const auto friday = shown(at("2026-10-30T20:00:00Z")); // 2026-10-30 is a Friday
    const auto kp = [](int64_t now) { return shown(now - 2 * kHour); };
    const auto xray = [](int64_t now) { return shown(now - kMinute); };
    for (const char* when : {"2026-11-02T09:00:00Z", "2026-11-02T20:00:00Z", "2026-11-02T23:59:00Z", "2026-11-03T08:00:00Z"}) {
        const int64_t now = at(when);
        check(is_live(space(kp(now), friday, xray(now), now)), "Friday SFI through Monday: Space Wx live");
        check(is_live(hf(friday, kp(now), now)), "Friday SFI through Monday: HF panes live");
    }
    const int64_t tuesday = at("2026-11-03T08:01:00Z");
    const auto c = hf(friday, kp(tuesday), tuesday);
    check(c.stale == 1 && std::string(c.note) == "SWPC · old SFI", "Monday's reading more than 12 h late: old SFI");
    check(c.oldest_ms == friday.time_ms, "footer age is the old reading's");
}

void one_input_old() {
    const int64_t now = at("2026-11-02T12:00:00Z");
    const auto kp = shown(now - 3 * kHour), sfi = shown(now - 16 * kHour), xray = shown(now - kMinute);
    const auto old_kp = shown(now - 10 * kHour), old_sfi = shown(now - 85 * kHour), old_xray = shown(now - 31 * kMinute);

    auto c = space(old_kp, sfi, xray, now);
    check(c.stale == 1 && std::string(c.note) == "SWPC · old Kp", "Space Wx: old Kp");
    check(std::string(c.short_note) == "old Kp" && std::string(c.tiny_note) == "old data", "old Kp, shorter forms");
    check(c.oldest_ms == old_kp.time_ms, "old Kp age");
    c = space(kp, old_sfi, xray, now);
    check(c.stale == 1 && std::string(c.note) == "SWPC · old SFI", "Space Wx: old SFI (never checked before)");
    check(c.oldest_ms == old_sfi.time_ms, "old SFI age");
    c = space(kp, sfi, old_xray, now);
    check(c.stale == 1 && std::string(c.note) == "SWPC · old X-ray", "Space Wx: old X-ray");
    check(std::string(hf(sfi, old_kp, now).note) == "SWPC · old Kp", "HF panes: old Kp");
    check(std::string(hf(old_sfi, kp, now).note) == "SWPC · old SFI", "HF panes: old SFI");

    // A feed past its fetch limit reads the same way, though its row is within the limit.
    c = hf(sfi, late_feed(now - 4 * kHour), now);
    check(c.stale == 1 && std::string(c.note) == "SWPC · old Kp" && c.oldest_ms == now - 4 * kHour, "late feed reads as old");
}

void all_old() {
    const int64_t now = at("2026-11-02T12:00:00Z");
    const auto kp = shown(now - 10 * kHour), sfi = shown(now - 100 * kHour), xray = shown(now - 45 * kMinute);

    auto c = space(kp, sfi, xray, now);
    check(c.stale == 1 && std::string(c.note) == "SWPC · old Kp, SFI, X-ray", "Space Wx: all old");
    check(std::string(c.short_note) == "old Kp, SFI, X-ray", "all old, short form");
    check(std::string(c.tiny_note) == "old data", "all old, tiny form");
    check(c.oldest_ms == sfi.time_ms, "age of the oldest old reading");

    c = hf(sfi, kp, now);
    check(c.stale == 1 && std::string(c.note) == "SWPC · old SFI, Kp", "HF panes: both old, main app's order");
    check(c.oldest_ms == sfi.time_ms, "HF panes: oldest reading");
}

void missing_and_waiting() {
    const int64_t now = at("2026-11-02T12:00:00Z");
    const auto kp = shown(now - 10 * kHour), sfi = shown(now - 100 * kHour), fresh_kp = shown(now - kHour), fresh_sfi = shown(now - kHour);

    // The longest note the main app can write fits the ABI buffer whole.
    auto c = space(kp, sfi, missing(), now);
    check(c.stale == 1 && std::string(c.note) == "SWPC · old Kp, SFI, no X-ray", "longest note complete");
    check(std::string(c.short_note) == "old Kp, SFI", "short form names only what is old");
    check(std::string(c.tiny_note) == "old data", "tiny form");

    c = space(fresh_kp, fresh_sfi, missing(), now);
    check(c.stale == 1 && std::string(c.note) == "SWPC · no X-ray" && c.oldest_ms == 0, "missing only: no reading age");
    check(std::string(c.short_note) == "no X-ray" && std::string(c.tiny_note) == "no data", "missing only, shorter forms");

    check(is_live(space(fresh_kp, fresh_sfi, waiting(), now)), "a feed not yet answered is neither old nor missing");
    check(is_live(space(waiting(), waiting(), waiting(), now)), "nothing yet: the fetch state speaks");
}

void real_fixture_times() {
    const auto kp_body = fixture("noaa-planetary-k-index.json");
    const auto kp = hcp_swpc_latest_kp(kp_body.data(), kp_body.size());
    const auto sfi_body = fixture("10cm-flux.json");
    const auto sfi = hcp_swpc_latest_solar_flux(sfi_body.data(), sfi_body.size());
    check(kp.valid && sfi.valid, "fixtures parse");
    const auto kp_in = shown(kp.time_ms), sfi_in = shown(sfi.time_ms);
    check(is_live(hf(sfi_in, kp_in, kp.time_ms + 8 * kHour)), "real tags within the limits: live");
    check(hf(sfi_in, kp_in, kp.time_ms + 10 * kHour).stale == 1, "real Kp tag past its limit: stale");
}

void bad_arguments() {
    HcpIndexInput in{};
    HcpIndexCheck out{};
    check(hcp_swpc_check_space_weather(nullptr, &in, &in, 0, &out) == 0 && hcp_swpc_check_hf(&in, &in, 0, nullptr) == 0, "NULL rejected");
}
} // namespace

int main() {
    limits();
    healthy_feeds_stay_live();
    friday_sfi_on_monday();
    one_input_old();
    all_old();
    missing_and_waiting();
    real_fixture_times();
    bad_arguments();
    return finish("index-age");
}
