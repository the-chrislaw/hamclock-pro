#pragma once
// Desktop window size and position, remembered between launches (Mac, Linux
// desktop). Kept in options.ini under [window]. A saved position on a display
// that is no longer connected is not restored, so the window never opens off-screen.
#include <QList>
#include <QRect>
#include <QSize>
#include <QString>
#include <optional>

class QWindow;

namespace hamclock {

// The geometry to restore: `saved` when enough of it (its top strip, where the
// title bar is) lies on one of `screens`, at least `minimum` in size; otherwise none.
std::optional<QRect> restorable_window_geometry(const QRect& saved, const QList<QRect>& screens,
                                                const QSize& minimum);

QRect load_window_geometry(const QString& options_path);
bool save_window_geometry(const QString& options_path, const QRect& geometry);

// Restores the saved geometry onto `window` (when restorable on the connected
// screens) and saves its last windowed geometry when the app quits.
void remember_window_geometry(QWindow* window, const QString& options_path);

} // namespace hamclock
