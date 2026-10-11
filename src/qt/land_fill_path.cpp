#include "land_fill_path.hpp"
#include <cmath>
#include <new>

namespace hamclock {
namespace {
LandFillResult failure(LandFillError error) { return {error,{}}; }
bool equal(LandFillPoint a,LandFillPoint b) {
    return a.longitude==b.longitude && a.latitude==b.latitude;
}
LandFillError validate(LandFillView view) {
    if (view.point_count>land_fill_max_points || view.ring_count>land_fill_max_rings ||
        view.record_count>land_fill_max_records) return LandFillError::CountLimit;
    if (!view.point_count || !view.ring_count || !view.record_count) return LandFillError::Empty;
    if (!view.points || !view.rings || !view.records) return LandFillError::InvalidView;
    std::size_t expected=0;
    for (std::size_t i=0;i<view.record_count;++i) {
        const auto record=view.records[i];
        if (record.first_ring!=expected || !record.count || record.count>view.ring_count-expected)
            return LandFillError::InvalidOffsets;
        expected+=record.count;
    }
    if (expected!=view.ring_count) return LandFillError::InvalidOffsets;
    expected=0;
    for (std::size_t i=0;i<view.ring_count;++i) {
        const auto ring=view.rings[i];
        if (ring.start!=expected || ring.count>view.point_count-expected)
            return LandFillError::InvalidOffsets;
        if (ring.count<4 || ring.count>land_fill_max_ring_points) return LandFillError::InvalidRing;
        expected+=ring.count;
    }
    if (expected!=view.point_count) return LandFillError::InvalidOffsets;
    for (std::size_t i=0;i<view.point_count;++i)
        if (!project_land_fill_point(view.points[i])) return LandFillError::InvalidCoordinate;
    for (std::size_t i=0;i<view.ring_count;++i) {
        const auto ring=view.rings[i];const auto* points=view.points+ring.start;
        if (!equal(points[0],points[ring.count-1])) return LandFillError::InvalidRing;
        std::optional<LandFillPoint> second;
        bool three_distinct=false;
        long double twice_area=0;
        for (std::size_t j=1;j<ring.count;++j) {
            const auto a=points[j-1],b=points[j];
            if (!equal(points[0],b)) {
                if (!second) second=b;
                else if (!equal(*second,b)) three_distinct=true;
            }
            if (std::abs(b.longitude-a.longitude)>180.) {
                const bool full_pole=std::abs(a.longitude)==180. && std::abs(b.longitude)==180. &&
                    a.latitude==b.latitude && std::abs(a.latitude)==90.;
                if (!full_pole) return LandFillError::UnsupportedSeam;
            }
            twice_area+=static_cast<long double>(a.longitude)*b.latitude-
                        static_cast<long double>(b.longitude)*a.latitude;
        }
        if (!three_distinct || twice_area==0) return LandFillError::InvalidRing;
    }
    return LandFillError::None;
}
}

std::optional<QPointF> project_land_fill_point(LandFillPoint point) noexcept {
    if (!std::isfinite(point.longitude) || !std::isfinite(point.latitude) ||
        point.longitude < -180. || point.longitude > 180. ||
        point.latitude < -90. || point.latitude > 90.) return std::nullopt;
    return QPointF((point.longitude+180.)/360.,(90.-point.latitude)/180.);
}

LandFillResult make_land_fill_paths(LandFillView view) {
    const auto error=validate(view);
    if (error!=LandFillError::None) return failure(error);
    try {
        LandFillResult result;
        result.paths.reserve(view.record_count);
        for (std::size_t i=0;i<view.record_count;++i) {
            QPainterPath path;path.setFillRule(Qt::WindingFill);
            const auto record=view.records[i];
            for (std::size_t j=0;j<record.count;++j) {
                const auto ring=view.rings[record.first_ring+j];
                path.moveTo(*project_land_fill_point(view.points[ring.start]));
                for (std::size_t k=1;k<ring.count;++k)
                    path.lineTo(*project_land_fill_point(view.points[ring.start+k]));
                path.closeSubpath();
            }
            result.paths.push_back(std::move(path));
        }
        return result;
    } catch (const std::bad_alloc&) {
        return failure(LandFillError::AllocationFailure);
    }
}
} // namespace hamclock
