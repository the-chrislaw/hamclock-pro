#include "world_map_widget.hpp"
#include "editor_appearance.hpp"
#include "../app/station_view.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyleHints>
#include <QVBoxLayout>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc,char** argv) {
    if (argc != 3 || std::string(argv[1]) != "--station" || std::string(argv[2]).empty()) {
        std::cerr << "Usage: hamclock-pro-map-preview --station PATH\n";
        return 2;
    }
    hamclock::StationSettings settings;
    hamclock::StationView station;
    try {
        settings=hamclock::load_settings(argv[2]);
        station=hamclock::make_station_view(settings);
    } catch (const std::exception& error) {
        std::cerr << "Map preview could not load station: " << error.what() << '\n';
        return 2;
    }
    int qt_argc=1;
    QApplication app(qt_argc,argv);
    QApplication::setApplicationName("HamClock Pro map preview");
    // Keep both preview controls keyboard reachable regardless of platform defaults.
    QApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusAllControls);
    QDialog dialog;
    dialog.setWindowTitle("HamClock Pro · Station map preview");
    dialog.setAccessibleName("Station map preview");
    dialog.resize(800,480);
    auto* layout=new QVBoxLayout(&dialog);
    layout->setContentsMargins(16,12,16,12);layout->setSpacing(8);
    auto* heading=new QLabel(QString::fromStdString(station.callsign)+" · Station map",&dialog);
    heading->setTextFormat(Qt::PlainText);
    QFont heading_font=heading->font();
    heading_font.setPointSizeF((heading_font.pointSizeF()>0?heading_font.pointSizeF():12)+3);
    heading_font.setBold(true);heading->setFont(heading_font);
    layout->addWidget(heading);
    auto* coordinates=new QLabel(QString("DE %1   ·   DX %2").arg(QString::fromStdString(station.de_grid),QString::fromStdString(station.dx_grid)),&dialog);
    coordinates->setObjectName("stationGridSummary");coordinates->setTextFormat(Qt::PlainText);
    coordinates->setAccessibleName("Station and destination Maidenhead grids: "+coordinates->text());
    coordinates->setWordWrap(true);layout->addWidget(coordinates);
    auto* map=new hamclock::WorldMapWidget(&dialog);
    map->setEndpoints(station.de,station.dx);
    auto* route_summary=new QLabel(map->routeSummary(),&dialog);
    route_summary->setObjectName("routeSummary");route_summary->setTextFormat(Qt::PlainText);
    route_summary->setWordWrap(true);
    route_summary->setAccessibleName("Computed short-path distance and bearing: "+route_summary->text());
    route_summary->setAccessibleDescription(map->accessibleDescription());
    layout->addWidget(route_summary);
    auto* controls=new QHBoxLayout;
    auto* appearance=new QComboBox(&dialog);
    appearance->setObjectName("mapAppearanceChoice");appearance->setAccessibleName("Map appearance");
    appearance->addItem("Follow system","system");appearance->addItem("Light","light");appearance->addItem("Dark","dark");
    appearance->setCurrentIndex(appearance->findData(QString::fromStdString(settings.appearance)));
    appearance->setMinimumHeight(44);
    auto* appearance_label=new QLabel("Appearance",&dialog);appearance_label->setBuddy(appearance);
    controls->addWidget(appearance_label);controls->addWidget(appearance);
    auto* route_visible=new QCheckBox("Show short path",&dialog);
    route_visible->setObjectName("routeVisibility");route_visible->setChecked(true);route_visible->setMinimumHeight(44);
    route_visible->setAccessibleDescription("Show or hide the computed short path. Station endpoints remain visible.");
    controls->addStretch();controls->addWidget(route_visible);
    QWidget::setTabOrder(appearance,route_visible);
    layout->addLayout(controls);layout->addWidget(map,1);
    auto* status=new QLabel("Offline coastline · Natural Earth 110m · Preview changes are not saved",&dialog);
    status->setTextFormat(Qt::PlainText);status->setWordWrap(true);
    layout->addWidget(status);
    auto update_appearance=[&](){
        const auto mode=appearance->currentData().toString();
        hamclock::apply_editor_appearance(dialog,mode);
        map->setAppearance(mode.toStdString());
    };
    QObject::connect(appearance,QOverload<int>::of(&QComboBox::currentIndexChanged),&dialog,[&](int){update_appearance();});
    QObject::connect(route_visible,&QCheckBox::toggled,map,[map,route_summary](bool visible){
        map->setRouteVisible(visible);
        route_summary->setAccessibleDescription(map->accessibleDescription());
    });
    update_appearance();
    appearance->setFocus();
    (void)dialog.exec();
    return 0;
}
