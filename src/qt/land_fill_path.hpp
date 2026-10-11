#pragma once

#include <QPainterPath>
#include <QPointF>
#include <cstddef>
#include <optional>
#include <vector>

namespace hamclock {

struct LandFillPoint { double longitude; double latitude; };
struct LandFillRing { std::size_t start; std::size_t count; };
struct LandFillRecord { std::size_t first_ring; std::size_t count; };

// Caller owns valid contiguous memory for every nonempty view, stable during
// the call. No input pointer is retained. Null pointers are rejected. This is
// an internal immutable-array API, not a runtime file or untrusted pointer API.
struct LandFillView {
    const LandFillPoint* points;
    std::size_t point_count;
    const LandFillRing* rings;
    std::size_t ring_count;
    const LandFillRecord* records;
    std::size_t record_count;
};

inline constexpr std::size_t land_fill_max_points = 100000;
inline constexpr std::size_t land_fill_max_rings = 10000;
inline constexpr std::size_t land_fill_max_records = 10000;
inline constexpr std::size_t land_fill_max_ring_points = 20000;

enum class LandFillError {
    None, Empty, InvalidView, CountLimit, InvalidOffsets,
    InvalidCoordinate, InvalidRing, UnsupportedSeam, AllocationFailure
};
struct LandFillResult {
    LandFillError error = LandFillError::None;
    // Exactly one WindingFill path per record on success; empty on failure.
    std::vector<QPainterPath> paths;
};

// Land-specific normalized planar projection. +180 -> x=1, -180 -> x=0;
// north -> y=0, south -> y=1. Finite strict geographic bounds; no snapping.
std::optional<QPointF> project_land_fill_point(LandFillPoint point) noexcept;

// Validates caps, contiguous complete coverage, closed nonzero-area rings
// with >=3 distinct vertices. Preserves all points/order; does not certify
// topology, repair contacts, classify holes or use shortest-edge wrapping.
// >180-degree longitude edges are rejected except exact +/-180 full-width
// edges at the same +/-90 pole. Input seam-cutting must be explicit.
// Paths use per-record WindingFill; caller controls brush and viewport clip.
// std::bad_alloc yields AllocationFailure with no partial paths.
LandFillResult make_land_fill_paths(LandFillView view);

} // namespace hamclock
