#pragma once

#include "spots/spot_event.hpp"
#include <vector>

namespace hamclock::spots {
inline constexpr std::size_t max_store_capacity = 10000;
enum class UpsertKind { Inserted, Updated };
struct UpsertResult { UpsertKind kind; std::optional<SpotKey> evicted; };

// Offline arrival-ordered model, no network/parser/map/hardware integration.
// Dedup is exact normalized source+event_identity, never cross-source callsign.
// This provisional storage key does not claim original latest-call/frequency parity.
// Updating replaces metadata in place without refreshing first-arrival order.
// Insertion at capacity evicts first arrival deterministically.
// Invalid inputs and allocation errors preserve the complete prior snapshot.
// Copies returned by snapshot cannot mutate store state. No clock/freshness,
// filter, scroll, persistence or synchronization state is implicitly maintained.
class SpotStore {
public:
    explicit SpotStore(std::size_t capacity);
    UpsertResult upsert(const SpotEvent& event);
    std::vector<SpotEvent> snapshot() const;
    std::size_t capacity() const noexcept;
    std::size_t size() const noexcept;
    void clear() noexcept;
private:
    std::size_t capacity_;
    std::vector<SpotEvent> events_;
};
} // namespace hamclock::spots
