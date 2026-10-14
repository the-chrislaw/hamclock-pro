#pragma once
// Safe-area insets (notch, home indicator, rounded corners) for the QML root.
// On iPad the native UIView reports them in points; everywhere else they stay
// zero, so layouts are unchanged on the Pi and macOS. QML binds its page
// margins to left/top/right/bottom.
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
public:
    explicit SafeArea(QObject* parent = nullptr) : QObject(parent) {}

    qreal left() const { return insets_.left(); }
    qreal top() const { return insets_.top(); }
    qreal right() const { return insets_.right(); }
    qreal bottom() const { return insets_.bottom(); }

    // Negative or non-finite values are treated as zero. Emits changed() only
    // when the sanitized value differs.
    void setInsets(const QMarginsF& insets);
    // Re-reads the native insets for `window` (no-op off iOS). Called when the
    // window is shown and whenever its size or the screen orientation changes.
    void attach(QWindow* window);
    void refresh();

signals:
    void changed();

private:
    QWindow* window_ = nullptr;
    QMarginsF insets_;
};

#if defined(Q_OS_IOS)
// Implemented in apple_safearea_ios.mm.
QMarginsF nativeSafeAreaInsets(QWindow* window);
#endif

} // namespace hamclock
