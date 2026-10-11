#include "instrument_catalog.hpp"

namespace hamclock {
namespace {
constexpr CatalogAvailability unavailable = CatalogAvailability::Unavailable;
constexpr std::array<InstrumentDescriptor, kKnownInstrumentCount> catalog{{
    {InstrumentId::SolarFlux, "solar-flux", "Solar flux", unavailable,
     "Solar flux observations, schema and source rights are not integrated."},
    {InstrumentId::SunspotNumber, "sunspot-number", "Sunspot number", unavailable,
     "Sunspot observations and their source rights are not integrated."},
    {InstrumentId::PlanetaryK, "planetary-k", "Planetary K", unavailable,
     "Planetary K observations and forecasts are not integrated."},
    {InstrumentId::XRay, "x-ray", "X-Ray", unavailable,
     "X-ray band observations, quality handling and source schema are not integrated."},
    {InstrumentId::SolarWind, "solar-wind", "Solar wind", unavailable,
     "Time-matched solar wind measurements and source schema are not integrated."},
    {InstrumentId::BzBt, "bz-bt", "Bz / Bt", unavailable,
     "Magnetic field observations and coordinate-frame metadata are not integrated."},
    {InstrumentId::Drap, "drap", "DRAP", unavailable,
     "Absorption model data and the scalar extraction definition are not integrated."},
    {InstrumentId::NoaaSpaceWeather, "noaa-space-weather", "NOAA SpaceWx", unavailable,
     "Space weather scales and their forecast schema are not integrated."},
    {InstrumentId::Aurora, "aurora", "Aurora", unavailable,
     "Aurora model grids and issue-time metadata are not integrated."},
    {InstrumentId::Disturbance, "disturbance", "Disturbance / Dst", unavailable,
     "Dst observations and independently reviewed publication rights are not integrated."},
    {InstrumentId::SolarImage, "solar-image", "Sun / SDO", unavailable,
     "Solar images, channel definitions and individual asset credits are not integrated."},
    {InstrumentId::Propagation, "propagation", "Propagation", unavailable,
     "A licensed propagation engine with independently verified outputs is not integrated."},
    {InstrumentId::Environment, "environment", "Environment", unavailable,
     "Sensor drivers, calibration and sensor availability are not integrated."},
    {InstrumentId::DeWeather, "de-weather", "DE weather", unavailable,
     "DE weather observations and provider permissions are not integrated."},
    {InstrumentId::DxWeather, "dx-weather", "DX weather", unavailable,
     "DX weather observations and provider permissions are not integrated."},
    {InstrumentId::DxCluster, "dx-cluster", "DX Cluster", unavailable,
     "A reviewed cluster protocol and spot pane adapter are not integrated."},
    {InstrumentId::LiveSpots, "live-spots", "Live spots", unavailable,
     "Live spot providers, permissions and pane adapters are not integrated."},
    {InstrumentId::OnTheAir, "on-the-air", "On the air", unavailable,
     "Activation providers, permissions and pane adapters are not integrated."},
    {InstrumentId::DxPeditions, "dxpeditions", "DXPeditions", unavailable,
     "DX expedition event data, permissions and pane adapters are not integrated."},
    {InstrumentId::Contests, "contests", "Contests", unavailable,
     "Contest event data, time rules and source permissions are not integrated."},
    {InstrumentId::Adif, "adif", "ADIF", unavailable,
     "A bounded local log importer and log pane adapter are not integrated."},
    {InstrumentId::Rotator, "rotator", "Rotator", unavailable,
     "Rotator feedback and an independently verified hardware command boundary are not integrated."},
    {InstrumentId::Countdown, "countdown", "Countdown", unavailable,
     "Countdown running, expiry and explicit restart semantics are not integrated."},
    {InstrumentId::Moon, "moon", "Moon", unavailable,
     "A reviewed astronomy algorithm and independent reference fixtures are not integrated."}
}};
} // namespace

const std::array<InstrumentDescriptor, kKnownInstrumentCount>&
instrument_catalog() noexcept {
    return catalog;
}

const InstrumentDescriptor* find_instrument(InstrumentId id) noexcept {
    for (const auto& descriptor : catalog) {
        if (descriptor.id == id) {
            return &descriptor;
        }
    }
    return nullptr;
}

bool InstrumentSelection::valid_slot(InstrumentSlot slot) noexcept {
    return static_cast<std::size_t>(slot) < kInstrumentSlotCount;
}

SelectionResult InstrumentSelection::assign(InstrumentSlot slot,
                                           InstrumentId id) noexcept {
    if (!valid_slot(slot)) {
        return SelectionResult::InvalidSlot;
    }
    if (find_instrument(id) == nullptr) {
        return SelectionResult::UnknownInstrument;
    }
    const auto index = static_cast<std::size_t>(slot);
    if (assignments_[index] == id) {
        return SelectionResult::Unchanged;
    }
    for (const auto& assigned : assignments_) {
        if (assigned == id) {
            return SelectionResult::AlreadyAssigned;
        }
    }
    assignments_[index] = id;
    return SelectionResult::Applied;
}

SelectionResult InstrumentSelection::clear(InstrumentSlot slot) noexcept {
    if (!valid_slot(slot)) {
        return SelectionResult::InvalidSlot;
    }
    auto& assigned = assignments_[static_cast<std::size_t>(slot)];
    if (!assigned.has_value()) {
        return SelectionResult::Unchanged;
    }
    assigned.reset();
    return SelectionResult::Applied;
}

std::optional<InstrumentId>
InstrumentSelection::selected(InstrumentSlot slot) const noexcept {
    if (!valid_slot(slot)) {
        return std::nullopt;
    }
    return assignments_[static_cast<std::size_t>(slot)];
}

const std::array<std::optional<InstrumentId>, kInstrumentSlotCount>&
InstrumentSelection::assignments() const noexcept {
    return assignments_;
}
} // namespace hamclock
