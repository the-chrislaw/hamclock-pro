#pragma once
// Safe-area insets (notch, home indicator, rounded corners) for the QML root.
// On iPad the native UIView reports them in points; everywhere else they stay
// zero, so layouts are unchanged on the Pi and macOS. QML binds its page
// margins to left/top/right/bottom. On iOS the window stays inside the safe
// area; `backdrop` is painted behind the status bar, home indicator and the
// landscape side insets (black bands by default), with status bar text that
// stays readable on it.
#include <QColor>
#include <QObject>
#include <QMarginsF>

class QWindow;

namespace hamclock {

class SafeArea : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal left READ left NOTIFY changed)
    Q_PROPERTY(qreal top READ top NOTIFY changed)
    Q_PROPERTY(qreal right READ right NOTIFY changed)
    Q_PROPERTY(qreal bottom READ bottom NOTIFY changed)
    Q_PROPERTY(QColor backdrop READ backdrop WRITE setBackdrop NOTIFY backdropChanged)
public:
    explicit SafeArea(QObject* parent = nullptr) : QObject(parent) {}

    qreal left() const { return insets_.left(); }
    qreal top() const { return insets_.top(); }
    qreal right() const { return insets_.right(); }
    qreal bottom() const { return insets_.bottom(); }

    // Negative or non-finite values are treated as zero. Emits changed() only
    // when the sanitized value differs.
    void setInsets(const QMarginsF& insets);
    QColor backdrop() const { return backdrop_; }
    // True when the backdrop is dark, so the status bar uses light text.
    bool darkBackdrop() const { return backdrop_.lightnessF() < 0.5; }
    // Invalid colours are ignored. Emits backdropChanged() only on a change.
    void setBackdrop(const QColor& color);
    // Re-reads the native insets for `window` (no-op off iOS). Called when the
    // window is shown and whenever its size or the screen orientation changes.
    void attach(QWindow* window);
    void refresh();

signals:
    void changed();
    void backdropChanged();

private:
    QWindow* window_ = nullptr;
    QMarginsF insets_;
    QColor backdrop_{Qt::black};
};

#if defined(Q_OS_IOS)
// Implemented in apple_safearea_ios.mm.
QMarginsF nativeSafeAreaInsets(QWindow* window);
// Paints the area around the window (outside the safe area) and picks status bar text.
void nativeApplyBackdrop(QWindow* window, const QColor& color, bool dark);
#endif

} // namespace hamclock
