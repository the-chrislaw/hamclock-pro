#include "spots/spot_store.hpp"
#include <algorithm>
#include <stdexcept>

namespace hamclock::spots {
SpotStore::SpotStore(std::size_t capacity) : capacity_(capacity) {
    if (capacity == 0 || capacity > max_store_capacity) throw std::invalid_argument("invalid spot capacity");
}
UpsertResult SpotStore::upsert(const SpotEvent& event) {
    auto validated = normalize_event(event);
    auto next = events_;
    auto existing = std::find_if(next.begin(), next.end(), [&](const SpotEvent& item) { return item.key == validated.key; });
    UpsertResult result{UpsertKind::Inserted, std::nullopt};
    if (existing != next.end()) {
        *existing = std::move(validated);
        result.kind = UpsertKind::Updated;
    } else {
        if (next.size() == capacity_) {
            result.evicted = next.front().key;
            next.erase(next.begin());
        }
        next.push_back(std::move(validated));
    }
    events_.swap(next);
    return result;
}
std::vector<SpotEvent> SpotStore::snapshot() const { return events_; }
std::size_t SpotStore::capacity() const noexcept { return capacity_; }
std::size_t SpotStore::size() const noexcept { return events_.size(); }
void SpotStore::clear() noexcept { events_.clear(); }
} // namespace hamclock::spots
