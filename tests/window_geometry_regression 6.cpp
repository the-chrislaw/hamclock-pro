// Desktop window geometry is remembered between launches, but never restored onto
// a display that is gone (unplugged external monitor) or with a corrupt size.
#include "../src/ui/window_geometry.hpp"
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QTemporaryDir>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    using hamclock::restorable_window_geometry;
    const QSize minimum(640, 400);
    const QRect laptop(0, 25, 1512, 957);       // built-in display below the menu bar
    const QRect external(1512, -200, 2560, 1415); // monitor to the right, raised
    const QList<QRect> both{laptop, external};

    const QRect on_laptop(100, 100, 1200, 700);
    check(restorable_window_geometry(on_laptop, both, minimum) == on_laptop, "window on the laptop is restored");
    const QRect on_external(2000, 0, 1800, 1000);
    check(restorable_window_geometry(on_external, both, minimum) == on_external, "window on the external monitor is restored");
    check(!restorable_window_geometry(on_external, {laptop}, minimum), "monitor unplugged: not restored off-screen");
    check(!restorable_window_geometry(QRect(100, -300, 1200, 700), {laptop}, minimum), "title bar above the screen: not restored");
    check(!restorable_window_geometry(QRect(1490, 100, 1200, 700), {laptop}, minimum), "only a sliver on screen: not restored");
    check(!restorable_window_geometry(QRect(), both, minimum), "nothing saved");
    check(!restorable_window_geometry(QRect(0, 30, 900000, 700), both, minimum), "corrupt size");
    const auto grown = restorable_window_geometry(QRect(100, 100, 300, 200), both, minimum);
    check(grown && grown->size() == minimum && grown->topLeft() == QPoint(100, 100), "too small grows to the minimum");

    QTemporaryDir dir;
    const QString ini = dir.filePath("options.ini");
    check(hamclock::load_window_geometry(ini).isNull(), "no file: nothing saved");
    check(hamclock::save_window_geometry(ini, on_external), "save");
    check(hamclock::load_window_geometry(ini) == on_external, "round trip");
    QFile garbage(ini);
    check(garbage.open(QIODevice::WriteOnly) && garbage.write("[window]\ngeometry=banana\n") > 0, "write garbage");
    garbage.close();
    check(!restorable_window_geometry(hamclock::load_window_geometry(ini), both, minimum), "garbage is ignored");

    // The app wiring: restore at start, save the last windowed geometry at quit,
    // even when the window was closed (hidden) before quitting.
    const QRect first_screen = QGuiApplication::primaryScreen()->availableGeometry();
    const QRect start(first_screen.topLeft() + QPoint(40, 40), QSize(700, 450));
    check(hamclock::save_window_geometry(ini, start), "save start geometry");
    QWindow window;
    window.setMinimumSize(minimum);
    window.resize(1024, 600);
    hamclock::remember_window_geometry(&window, ini);
    check(window.geometry() == start, "saved geometry restored at start");
    window.show();
    const QRect moved(first_screen.topLeft() + QPoint(60, 50), QSize(800, 500));
    window.setGeometry(moved);
    QCoreApplication::processEvents(); // the platform reports the move
    window.hide(); // closed before quitting
    QTimer::singleShot(0, &app, &QCoreApplication::quit);
    app.exec();
    check(hamclock::load_window_geometry(ini) == moved, "last windowed geometry saved at quit");

    std::cout << (failures ? "FAILED\n" : "window-geometry-regression: ok\n");
    return failures ? 1 : 0;
}
