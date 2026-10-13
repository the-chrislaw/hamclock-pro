#pragma once
#include "../platform/settings.hpp"
#include <array>
#include <string>

namespace hamclock {
enum class StationField { Callsign, DeLatitude, DeLongitude, DxLatitude, DxLongitude, Appearance };
class StationEdit {
public:
    explicit StationEdit(const StationSettings&);
    bool set(StationField, const std::string&);
    std::string get(StationField) const;
    bool dirty() const;
    void cancel();
    StationSettings validated() const;
private:
    std::string de_zone_; // Not editable here; carried through so saving keeps it.
    std::array<std::string, 6> original_;
    std::array<std::string, 6> draft_;
};
int run_station_edit_tests();
}
