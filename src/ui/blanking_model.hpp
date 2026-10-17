#pragma once
#include "blanking.hpp"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace hamclock {

// QML face of the screen blanking: `blanked` drives a black overlay, touch() wakes it.
class BlankingModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool blanked READ blanked NOTIFY blankedChanged)
public:
    explicit BlankingModel(QObject* parent = nullptr);
    bool blanked() const { return blanked_; }
    void setConfig(const BlankingConfig& config);
    Q_INVOKABLE void touch();
    // Test hook: evaluate with an explicit local time and idle time.
    void evaluate(const QTime& local, qint64 idle_secs);
    void evaluate(const QDateTime& local, qint64 idle_secs);
signals:
    void blankedChanged();
private:
    BlankingConfig config_;
    QElapsedTimer since_touch_;
    QTimer timer_;
    bool blanked_ = false;
};

}  // namespace hamclock
