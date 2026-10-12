#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace hamclock {

// Original HamClock Pro identifiers for known manual-documented families.
// These explicit values are stable within this replacement; they are not the
// original program's enum values or a certified exhaustive parity inventory.
enum class InstrumentId : std::uint16_t {
    SolarFlux = 1,
    SunspotNumber = 2,
    PlanetaryK = 3,
    XRay = 4,
    SolarWind = 5,
    BzBt = 6,
    Drap = 7,
    NoaaSpaceWeather = 8,
    Aurora = 9,
    Disturbance = 10,
    SolarImage = 11,
    Propagation = 12,
    Environment = 13,
    DeWeather = 14,
    DxWeather = 15,
    DxCluster = 16,
    LiveSpots = 17,
    OnTheAir = 18,
    DxPeditions = 19,
    Contests = 20,
    Adif = 21,
    Rotator = 22,
    Countdown = 23,
    Moon = 24
};

// This offline catalog supplies no readings, providers or integrated modules.
// Runtime loading/current/stale states belong to a separate snapshot contract.
enum class CatalogAvailability { Unavailable };

struct InstrumentDescriptor {
    InstrumentId id;
    const char* key; // Stable ASCII program key; never an executable URL.
    const char* label; // Plain text, not HTML or a claim of a live reading.
    CatalogAvailability availability;
    const char* unavailable_reason;
};

constexpr std::size_t kKnownInstrumentCount = 24;
constexpr std::size_t kInstrumentSlotCount = 3;

const std::array<InstrumentDescriptor, kKnownInstrumentCount>&
instrument_catalog() noexcept;

// Null for any value not explicitly present, including cast-in unknown IDs.
const InstrumentDescriptor* find_instrument(InstrumentId) noexcept;

enum class InstrumentSlot : std::uint8_t { First = 0, Second = 1, Third = 2 };
enum class SelectionResult {
    Applied,
    Unchanged,
    InvalidSlot,
    UnknownInstrument,
    AlreadyAssigned
};

// Bounded provisional single-choice configuration model, NOT the original
// multiselect/rotation engine. Empty initial slots and clear() are replacement
// preparation states, not asserted original defaults. Known unavailable entries
// may be configured; selecting them does not make their modules available.
// Duplicate assignment is rejected atomically because the manual describes
// exclusive pane assignments. No automatic transfer/swap behavior is inferred.
// Utility strip, overlays, rotation, persistence and Countdown running/restart
// semantics remain separate, unresolved integration contracts.
class InstrumentSelection {
public:
    InstrumentSelection() noexcept = default;

    SelectionResult assign(InstrumentSlot, InstrumentId) noexcept;
    SelectionResult clear(InstrumentSlot) noexcept;

    // Invalid slot returns nullopt, just as an empty slot does. Callers needing
    // to distinguish those conditions must first call valid_slot().
    std::optional<InstrumentId> selected(InstrumentSlot) const noexcept;
    const std::array<std::optional<InstrumentId>, kInstrumentSlotCount>&
    assignments() const noexcept;
    static bool valid_slot(InstrumentSlot) noexcept;

private:
    std::array<std::optional<InstrumentId>, kInstrumentSlotCount> assignments_{};
};

} // namespace hamclock
