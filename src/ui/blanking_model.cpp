#include "blanking_model.hpp"

namespace hamclock {

BlankingModel::BlankingModel(QObject* parent) : QObject(parent) {
    since_touch_.start();
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, [this] { evaluate(QDateTime::currentDateTime(), since_touch_.elapsed() / 1000); });
    timer_.start();
}

void BlankingModel::setConfig(const BlankingConfig& config) {
    if (!blanking_config_valid(config)) return;
    config_ = config;
    touch();
}

void BlankingModel::touch() {
    since_touch_.restart();
    evaluate(QDateTime::currentDateTime(), 0);
}

void BlankingModel::evaluate(const QTime& local, qint64 idle_secs) {
    evaluate(QDateTime(QDate::currentDate(), local), idle_secs);
}

void BlankingModel::evaluate(const QDateTime& local, qint64 idle_secs) {
    const bool now = should_blank(config_, local.date(), local.time(), idle_secs);
    if (now == blanked_) return;
    blanked_ = now;
    emit blankedChanged();
}

}  // namespace hamclock
