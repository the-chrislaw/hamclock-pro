#include "window_geometry.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QWindow>
#include <algorithm>
#include <memory>

namespace hamclock {
namespace {
// How much of the window's top strip must be on a screen to be reachable with the mouse.
constexpr int kTitleStripHeight = 40;
constexpr int kMinVisibleWidth = 100;
constexpr int kMaxSide = 32768; // larger than any display arrangement; rejects corrupt values
} // namespace

std::optional<QRect> restorable_window_geometry(const QRect& saved, const QList<QRect>& screens,
                                                const QSize& minimum) {
    if (!saved.isValid() || saved.width() > kMaxSide || saved.height() > kMaxSide) return std::nullopt;
    const QRect sized(saved.topLeft(), saved.size().expandedTo(minimum));
    const QRect title(sized.left(), sized.top(), sized.width(), std::min(kTitleStripHeight, sized.height()));
    const bool reachable = std::any_of(screens.begin(), screens.end(), [&title](const QRect& screen) {
        const QRect on = screen.intersected(title);
        return on.width() >= std::min(kMinVisibleWidth, title.width()) && on.top() == title.top();
    });
    if (!reachable) return std::nullopt;
    return sized;
}

QRect load_window_geometry(const QString& options_path) {
    const QSettings ini(options_path, QSettings::IniFormat);
    return ini.value("window/geometry").toRect();
}

bool save_window_geometry(const QString& options_path, const QRect& geometry) {
    QSettings ini(options_path, QSettings::IniFormat);
    ini.setValue("window/geometry", geometry);
    ini.sync();
    return ini.status() == QSettings::NoError;
}

void remember_window_geometry(QWindow* window, const QString& options_path) {
    if (!window) return;
    QList<QRect> screens;
    for (const auto* screen : QGuiApplication::screens()) screens << screen->availableGeometry();
    const auto restored = restorable_window_geometry(load_window_geometry(options_path), screens, window->minimumSize());
    if (restored) window->setGeometry(*restored);
    // Track the last windowed geometry: at quit the window may already be hidden
    // (closed with the red button) or full screen, whose geometry is not wanted.
    auto last = std::make_shared<QRect>(window->geometry());
    const auto track = [window, last] {
        if (window->visibility() == QWindow::Windowed) *last = window->geometry();
    };
    for (const auto signal : {&QWindow::xChanged, &QWindow::yChanged, &QWindow::widthChanged, &QWindow::heightChanged})
        QObject::connect(window, signal, window, track);
    QObject::connect(qGuiApp, &QCoreApplication::aboutToQuit, window, [options_path, last] {
        if (!save_window_geometry(options_path, *last)) qWarning("Could not save the window size.");
    });
}

} // namespace hamclock
