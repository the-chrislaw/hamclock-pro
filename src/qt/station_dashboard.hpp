#pragma once
#include "../platform/settings.hpp"
#include "../app/stopwatch_presentation.hpp"
#include <QElapsedTimer>
#include <QDialog>
#include <string>
class QLabel;class QTimer;class QComboBox;class QPushButton;class QFrame;class QHBoxLayout;class QVBoxLayout;class QResizeEvent;
namespace hamclock {
class WorldMapWidget;
class StationDashboard : public QDialog {
public:
 StationDashboard(const StationSettings& initial,const std::string& path,QWidget* parent=nullptr);
 const StationSettings& stationSettings() const noexcept {return settings_;}
protected:
 void resizeEvent(QResizeEvent*) override;
private:
 void refreshStation();void editStation();void adaptLayout();void showStopwatch();
 StationSettings settings_;std::string path_;
 QLabel *callsign_=nullptr,*de_grid_=nullptr,*dx_grid_=nullptr,*de_coordinates_=nullptr,*dx_coordinates_=nullptr;
 QLabel *distance_=nullptr,*bearing_=nullptr,*route_summary_=nullptr,*clock_=nullptr,*date_=nullptr;
 WorldMapWidget* map_=nullptr;QTimer* clock_timer_=nullptr;
 QComboBox* appearance_=nullptr;QPushButton* route_toggle_=nullptr;
 QFrame *sidebar_=nullptr,*instruments_=nullptr;
 QVBoxLayout* outer_=nullptr;QHBoxLayout* body_=nullptr;
 bool route_visible_=true;
 QElapsedTimer stopwatch_clock_;
 StopwatchState stopwatch_state_{MonotonicNanoseconds{0}};
 StopwatchPresentation stopwatch_presentation_;
};
}
