// Original HamClock Pro application code, covered by the repository MIT license.
#include "station_dashboard.hpp"
#include "../app/station_view.hpp"
#include <QApplication>
#include <QStyleHints>
#include <exception>
#include <iostream>
#include <string>

int main(int argc,char** argv) {
    if(argc!=3 || std::string(argv[1])!="--station") {
        std::cerr<<"Usage: hamclock-pro-dashboard --station PATH\n";
        return 2;
    }
    const std::string path=argv[2];
    hamclock::StationSettings snapshot;
    try {
        snapshot=hamclock::load_settings(path);
        (void)hamclock::make_station_view(snapshot);
    } catch(const std::exception& error) {
        std::cerr<<"Dashboard could not open station settings: "<<error.what()<<'\n';
        return 2;
    }
    // Validate before GUI startup. Pass only the executable name to Qt so
    // station arguments cannot be interpreted as plugin or stylesheet options.
    int qt_argc=1;
    QApplication app(qt_argc,argv);
    QApplication::setApplicationName("HamClock Pro dashboard");
    QApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusAllControls);
    try {
        hamclock::StationDashboard dashboard(snapshot,path);
        dashboard.show();
        return app.exec();
    } catch(const std::exception& error) {
        std::cerr<<"Dashboard could not start: "<<error.what()<<'\n';
        return 2;
    }
}
