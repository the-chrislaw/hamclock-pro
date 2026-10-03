#pragma once
#include "../platform/settings.hpp"
#include <QDialog>
#include <string>
class QLabel;
class QTimer;
namespace hamclock {
class WorldMapWidget;
class StationDashboard : public QDialog {
public:
    StationDashboard(const StationSettings& initial,const std::string& path,QWidget* parent=nullptr);
    const StationSettings& stationSettings() const noexcept {return settings_;}
private:
    void refreshStation();
    void editStation();
    StationSettings settings_;
    std::string path_;
    QLabel *callsign_=nullptr,*de_grid_=nullptr,*dx_grid_=nullptr,*de_coordinates_=nullptr,*dx_coordinates_=nullptr;
    QLabel *distance_=nullptr,*bearing_=nullptr,*route_summary_=nullptr,*clock_=nullptr;
    WorldMapWidget* map_=nullptr;
    QTimer* clock_timer_=nullptr;
};
}
