#include "apple_safearea.hpp"

#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <cmath>

namespace hamclock {

namespace {
qreal sane(qreal v) { return std::isfinite(v) && v > 0 ? v : 0; }
} // namespace

void SafeArea::setInsets(const QMarginsF& insets) {
    const QMarginsF clean(sane(insets.left()), sane(insets.top()), sane(insets.right()), sane(insets.bottom()));
    if (clean == insets_) return;
    insets_ = clean;
    emit changed();
}

void SafeArea::attach(QWindow* window) {
    if (window_ == window) return;
    if (window_) disconnect(window_, nullptr, this, nullptr);
    window_ = window;
    if (!window_) return;
    connect(window_, &QWindow::widthChanged, this, &SafeArea::refresh);
    connect(window_, &QWindow::heightChanged, this, &SafeArea::refresh);
    connect(window_, &QWindow::visibleChanged, this, &SafeArea::refresh);
    connect(window_, &QWindow::screenChanged, this, &SafeArea::refresh);
    refresh();
}

void SafeArea::refresh() {
#if defined(Q_OS_IOS)
    if (window_) setInsets(nativeSafeAreaInsets(window_));
#endif
}

} // namespace hamclock
