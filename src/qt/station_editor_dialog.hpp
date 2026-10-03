#pragma once
#include "../platform/settings.hpp"
#include <optional>
#include <string>
class QWidget;
namespace hamclock {
// Runs an actual modal editor. A value is returned only after successful
// persistence. Cancel/Escape/close return no value; Save failures retain draft.
std::optional<StationSettings> show_station_editor(QWidget* parent,const std::string& path,
                                                  const StationSettings& initial,bool creating);
}
