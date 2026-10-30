#include "widget_model.hpp"
#include "../net/polled_source.hpp"
#include <stdexcept>

namespace hamclock {
namespace {
constexpr int kAgeTickMs = 30000;
}

WidgetModel::WidgetModel(QString id, QString title, QObject* parent)
    : QObject(parent), id_(std::move(id)), title_(std::move(title)),
      clock_([] { return QDateTime::currentDateTimeUtc(); }) {
    if (id_.isEmpty() || title_.isEmpty()) throw std::invalid_argument("Widget id and title required");
    age_timer_.setInterval(kAgeTickMs);
    connect(&age_timer_, &QTimer::timeout, this, &WidgetModel::ageChanged);
    age_timer_.start();
}

QString WidgetModel::ageText() const { return formatAge(updated_at_, clock_()); }

QString WidgetModel::formatAge(const QDateTime& then_utc, const QDateTime& now_utc) {
    if (!then_utc.isValid()) return {};
    const auto seconds = then_utc.secsTo(now_utc);
    if (seconds < 90) return "just now";
    if (seconds < 90 * 60) return QString("%1 min ago").arg((seconds + 30) / 60);
    if (seconds < 36 * 3600) return QString("%1 h ago").arg((seconds + 1800) / 3600);
    return QString("%1 days ago").arg((seconds + 43200) / 86400);
}

void WidgetModel::setState(const QString& state, const QString& status_text, const QDateTime& updated_at) {
    if (state == state_ && status_text == status_text_ && updated_at == updated_at_) return;
    state_ = state;
    status_text_ = status_text;
    updated_at_ = updated_at;
    emit stateChanged();
    emit ageChanged();
}

bool WidgetModel::awaitingFirstAnswer(const PolledSource& source) {
    return !source.fetchedAt().isValid() && source.lastError().isEmpty();
}

void WidgetModel::trackSource(PolledSource& source, const QString& source_label) {
    auto refresh = [this, &source, source_label]() {
        const auto now_utc = clock_();
        if (!source.fetchedAt().isValid()) {
            if (source.lastError().isEmpty()) setState("loading", "Loading " + source_label + "…");
            else setState("offline", "No data from " + source_label + ": " + source.lastError());
            return;
        }
        if (source.isStale(now_utc)) {
            setState("stale", source_label + " data is out of date" +
                     (source.lastError().isEmpty() ? QString() : " (" + source.lastError() + ")"),
                     source.fetchedAt());
            return;
        }
        setState("live", source_label, source.fetchedAt());
    };
    connect(&source, &PolledSource::updated, this, refresh);
    connect(&source, &PolledSource::failed, this, refresh);
    connect(&age_timer_, &QTimer::timeout, this, refresh);
}

} // namespace hamclock
