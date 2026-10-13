#include "station_editor_dialog.hpp"
#include <QApplication>
#include <QStyleHints>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

namespace {
void require_new_destination(const std::string& path) {
    struct stat status {};
    if (::lstat(path.c_str(), &status) == 0)
        throw std::runtime_error("New station destination already exists; use --station to edit it");
    if (errno != ENOENT)
        throw std::runtime_error(std::string("Inspect new station destination: ") + std::strerror(errno));
}

}

int main(int argc, char** argv) {
    if (argc != 3 || (std::string(argv[1]) != "--station" && std::string(argv[1]) != "--new-station")) {
        std::cerr << "Usage: hamclock-pro-editor --station PATH\n"
                     "       hamclock-pro-editor --new-station PATH\n";
        return 2;
    }
    const bool creating = std::string(argv[1]) == "--new-station";
    const std::string path = argv[2];
    if (path.empty()) { std::cerr << "A settings path is required\n"; return 2; }
    hamclock::StationSettings initial;
    try {
        if (creating) {
            require_new_destination(path);
            initial = {{0,0},{0,0},"N0CALL","system",""};
        } else initial = hamclock::load_settings(path);
    } catch (const std::exception& error) {
        std::cerr << "Station editor could not open settings: " << error.what() << '\n';
        return 2;
    }
    // No user-selected Qt plugin options are interpreted by this explicit CLI.
    int qt_argc = 1;
    QApplication app(qt_argc,argv);
    QApplication::setApplicationName("HamClock Pro station editor");
    QApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusAllControls);
    (void)hamclock::show_station_editor(nullptr,path,initial,creating);
    return 0;
}
