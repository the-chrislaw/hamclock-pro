#pragma once
// Base for every dashboard widget's data model. A widget = one folder under
// src/widgets/<id>/ holding its model (subclass of WidgetModel), its QML card
// and its tests, registered with hamclock_add_widget() in its CMakeLists.txt.
// See docs/WIDGETS.md for the contract.
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>
#include <functional>

namespace hamclock {

class AppSettings;
class PolledSource;

class WidgetModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString widgetId READ widgetId CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    // One of: unconfigured, loading, live, stale, offline, error.
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt NOTIFY stateChanged)
    Q_PROPERTY(QString ageText READ ageText NOTIFY ageChanged)
public:
    WidgetModel(QString id, QString title, QObject* parent = nullptr);

    QString widgetId() const { return id_; }
    QString title() const { return title_; }
    QString state() const { return state_; }
    QString statusText() const { return status_text_; }
    QDateTime updatedAt() const { return updated_at_; }
    QString ageText() const;

    // Human age of data, e.g. "just now", "4 min ago", "3 h ago".
    static QString formatAge(const QDateTime& then_utc, const QDateTime& now_utc);
    void setClock(std::function<QDateTime()> clock) { clock_ = std::move(clock); }
    QDateTime now() const { return clock_(); }

signals:
    void stateChanged();
    void ageChanged();

protected:
    void setState(const QString& state, const QString& status_text, const QDateTime& updated_at = {});
    // Standard mapping for a widget backed by one PolledSource: live when fresh,
    // stale when older than the policy, offline/error with the last error text.
    void trackSource(PolledSource& source, const QString& source_label);

private:
    QString id_, title_;
    QString state_ = "loading";
    QString status_text_ = "Loading…";
    QDateTime updated_at_;
    QTimer age_timer_;
    std::function<QDateTime()> clock_;
};

} // namespace hamclock
