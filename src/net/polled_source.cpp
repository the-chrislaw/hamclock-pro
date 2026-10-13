#include "polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <algorithm>
#include <stdexcept>

namespace hamclock {
namespace {
constexpr int kTimeoutMs = 20000;
constexpr qint64 kMaxBodyBytes = 4 * 1024 * 1024;
constexpr std::chrono::seconds kFirstRetry{60};

QString cache_dir() {
    auto dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.isEmpty()) dir = QDir::tempPath() + "/hamclock-pro-cache";
    return dir + "/sources";
}
} // namespace

QByteArray hamclock_user_agent() {
    auto version = QCoreApplication::applicationVersion();
    if (version.isEmpty()) version = "dev";
    return QByteArray("HamClockPro/") + version.toUtf8() +
        " (+https://github.com/the-chrislaw/hamclock-pro)";
}

Transport make_network_transport(QObject* owner) {
    auto* manager = new QNetworkAccessManager(owner);
    return [manager](const QUrl& url, FetchDone done) {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, hamclock_user_agent());
        request.setTransferTimeout(kTimeoutMs);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        auto* reply = manager->get(request);
        QObject::connect(reply, &QNetworkReply::finished, reply, [reply, done = std::move(done)]() {
            FetchResult result;
            result.http_status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (reply->error() != QNetworkReply::NoError) {
                result.error = reply->errorString();
            } else {
                result.body = reply->read(kMaxBodyBytes + 1);
                if (result.body.size() > kMaxBodyBytes) {
                    result.body.clear();
                    result.error = "Response too large";
                }
            }
            reply->deleteLater();
            done(std::move(result));
        });
    };
}

PolledSource::PolledSource(QString cache_name, QUrl url, PollPolicy policy, Transport transport,
                           QObject* parent)
    : QObject(parent), cache_name_(std::move(cache_name)), url_(std::move(url)), policy_(policy),
      transport_(std::move(transport)), clock_([] { return QDateTime::currentDateTimeUtc(); }) {
    static const QRegularExpression safe("^[a-z0-9-]{1,64}$");
    if (!safe.match(cache_name_).hasMatch()) throw std::invalid_argument("Invalid cache name");
    if (!url_.isValid() || url_.scheme() != "https") throw std::invalid_argument("Source URL must be https");
    if (!transport_) throw std::invalid_argument("Transport required");
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, &PolledSource::fetch);
}

QString PolledSource::cachePath() const { return cache_dir() + "/" + cache_name_ + ".cache"; }

void PolledSource::start() {
    loadCache();
    fetch();
}

void PolledSource::refreshNow() {
    timer_.stop();
    fetch();
}

bool PolledSource::isStale(const QDateTime& now_utc) const {
    if (!fetched_at_.isValid()) return true;
    return fetched_at_.secsTo(now_utc) > policy_.stale_after.count();
}

std::chrono::seconds PolledSource::nextDelay() const {
    if (failures_ == 0) return policy_.interval;
    auto delay = kFirstRetry;
    for (int i = 1; i < failures_ && delay < policy_.max_backoff; ++i) delay *= 2;
    return std::min({delay, policy_.max_backoff, policy_.interval});
}

void PolledSource::fetch() {
    if (in_flight_) return;
    in_flight_ = true;
    QPointer<PolledSource> self(this);
    transport_(url_, [self](FetchResult result) {
        if (self) self->handle(std::move(result));
    });
}

void PolledSource::handle(FetchResult result) {
    in_flight_ = false;
    const bool ok = result.error.isEmpty() && result.http_status >= 200 && result.http_status < 300 &&
        !result.body.isEmpty();
    if (!ok) {
        ++failures_;
        last_error_ = !result.error.isEmpty() ? result.error
            : result.http_status ? QString("HTTP %1").arg(result.http_status) : QString("Empty response");
        schedule(nextDelay());
        emit failed(last_error_);
        return;
    }
    failures_ = 0;
    last_error_.clear();
    body_ = std::move(result.body);
    fetched_at_ = clock_();
    saveCache();
    schedule(policy_.interval);
    emit updated(false);
}

void PolledSource::schedule(std::chrono::seconds delay) {
    timer_.start(static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(delay).count()));
}

// Cache format: first line ISO-8601 UTC fetch time, then the raw body.
void PolledSource::loadCache() {
    QFile file(cachePath());
    if (!file.open(QIODevice::ReadOnly) || file.size() > kMaxBodyBytes + 64) return;
    const auto stamp = QDateTime::fromString(QString::fromLatin1(file.readLine().trimmed()), Qt::ISODate);
    auto body = file.readAll();
    if (!stamp.isValid() || body.isEmpty()) return;
    body_ = std::move(body);
    fetched_at_ = stamp.toUTC();
    emit updated(true);
}

void PolledSource::saveCache() {
    if (!QDir().mkpath(cache_dir())) return; // cache is best effort; live data still shown
    QSaveFile file(cachePath());
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(fetched_at_.toString(Qt::ISODate).toLatin1() + "\n");
    file.write(body_);
    file.commit();
}

} // namespace hamclock
