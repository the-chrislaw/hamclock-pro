#include "polled_source.hpp"
#include "../platform/apple_lifecycle.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkInformation>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimeZone>
#include <memory>
#include <algorithm>
#include <stdexcept>

namespace hamclock {
namespace {
constexpr qint64 kMaxBodyBytes = TransportLimits{}.max_bytes;
constexpr std::chrono::seconds kFirstRetry{60};
constexpr std::chrono::seconds kMaxRetryAfter{24 * 3600};
// A rate-limited source never waits longer than this between tries.
constexpr std::chrono::seconds kMaxRateLimitWait{6 * 3600};
constexpr qsizetype kHtmlSniffBytes = 512;

QString cache_dir() {
#if defined(Q_OS_MACOS)
    // Qt honours XDG_CACHE_HOME on Linux only; follow it on the Mac too so tests and
    // side-by-side builds can use their own cache instead of ~/Library/Caches.
    const QString xdg = qEnvironmentVariable("XDG_CACHE_HOME");
    if (QDir::isAbsolutePath(xdg)) return xdg + "/" + QCoreApplication::applicationName() + "/sources";
#endif
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

QString describe_http_status(int status) {
    const QString code = QString("HTTP %1").arg(status);
    if (status == 429) return code + " (rate limited by the server)";
    if (status == 401) return code + " (not authorized)";
    if (status == 403) return code + " (access refused by the server)";
    if (status == 404 || status == 410) return code + " (not found; the feed may have moved)";
    if (status >= 500 && status < 600) return code + " (server error; try again later)";
    return code;
}

QString describe_network_error(int network_error) {
    switch (static_cast<QNetworkReply::NetworkError>(network_error)) {
    case QNetworkReply::HostNotFoundError: return "Server not found (DNS lookup failed; check the network)";
    case QNetworkReply::TimeoutError:
    case QNetworkReply::OperationCanceledError: return "Timed out";
    case QNetworkReply::ConnectionRefusedError: return "Connection refused by the server";
    case QNetworkReply::RemoteHostClosedError: return "Connection closed by the server";
    case QNetworkReply::SslHandshakeFailedError:
        return "Secure connection failed (check the date and time, or a network sign-in page)";
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError: return "No network connection";
    case QNetworkReply::TooManyRedirectsError:
    case QNetworkReply::InsecureRedirectError: return "Redirected away from the feed (network sign-in page?)";
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
    case QNetworkReply::ProxyAuthenticationRequiredError: return "Proxy error";
    default: return "Network error";
    }
}

std::chrono::seconds parse_retry_after(const QByteArray& value, const QDateTime& now_utc) {
    const QByteArray text = value.trimmed();
    if (text.isEmpty()) return std::chrono::seconds{0};
    bool numeric = false;
    const qlonglong secs = text.toLongLong(&numeric);
    qint64 delta = 0;
    if (numeric) {
        delta = secs;
    } else {
        // IMF-fixdate, e.g. "Thu, 29 Oct 2026 12:10:00 GMT".
        QDateTime when = QLocale::c().toDateTime(QString::fromLatin1(text), "ddd, dd MMM yyyy HH:mm:ss 'GMT'");
        if (!when.isValid()) return std::chrono::seconds{0};
        when.setTimeZone(QTimeZone::utc());
        delta = now_utc.secsTo(when);
    }
    if (delta <= 0) return std::chrono::seconds{0};
    return std::min(std::chrono::seconds{delta}, kMaxRetryAfter);
}

bool looks_like_html(const QByteArray& body) {
    QByteArray head = body.left(kHtmlSniffBytes);
    if (head.startsWith("\xEF\xBB\xBF")) head = head.mid(3);
    head = head.trimmed().toLower();
    // Pages often open with <!-- comments --> before the doctype.
    while (head.startsWith("<!--")) {
        const auto end = head.indexOf("-->");
        if (end < 0) return true;
        head = head.mid(end + 3).trimmed();
    }
    return head.startsWith("<!doctype html") || head.startsWith("<html") || head.startsWith("<head") ||
        head.startsWith("<body") || head.startsWith("<title") || head.startsWith("<meta");
}

FetchResult fetch_result_from_reply(QNetworkReply& reply, const QString& abort_reason, qint64 max_bytes) {
    FetchResult result;
    result.http_status = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.retry_after = parse_retry_after(reply.rawHeader("Retry-After"), QDateTime::currentDateTimeUtc());
    if (!abort_reason.isEmpty()) {
        result.http_status = 0;
        result.error = abort_reason;
    } else if (reply.error() != QNetworkReply::NoError) {
        // Qt's errorString() repeats the URL (callsign, query); never show it.
        result.error = result.http_status >= 400 ? describe_http_status(result.http_status)
                                                 : describe_network_error(reply.error());
    } else {
        result.body = reply.read(max_bytes + 1);
        if (result.body.size() > max_bytes) {
            result.body.clear();
            result.error = "Response too large";
        }
    }
    return result;
}

AuthTransport make_authorized_transport(QObject* owner, TransportLimits limits) {
    auto* manager = new QNetworkAccessManager(owner);
    return [manager, limits](const QUrl& url, const QByteArray& bearer, FetchDone done) {
        QNetworkRequest request(url);
        if (!bearer.isEmpty()) request.setRawHeader("Authorization", "Bearer " + bearer);
        request.setHeader(QNetworkRequest::UserAgentHeader, hamclock_user_agent());
        request.setTransferTimeout(limits.inactivity_ms);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        auto* reply = manager->get(request);
        // Why we aborted, if we did: abort() reports OperationCanceledError either way.
        auto abort_reason = std::make_shared<QString>();
        auto* deadline = new QTimer(reply);
        deadline->setSingleShot(true);
        QObject::connect(deadline, &QTimer::timeout, reply, [reply, abort_reason] {
            *abort_reason = "Timed out";
            reply->abort();
        });
        deadline->start(limits.total_ms);
        QObject::connect(reply, &QNetworkReply::downloadProgress, reply,
                         [reply, abort_reason, max = limits.max_bytes](qint64 received, qint64 total) {
            if (received > max || total > max) {
                if (abort_reason->isEmpty()) *abort_reason = "Response too large";
                reply->abort();
            }
        });
        QObject::connect(reply, &QNetworkReply::finished, reply,
                         [reply, abort_reason, max = limits.max_bytes, done = std::move(done)]() {
            FetchResult result = fetch_result_from_reply(*reply, *abort_reason, max);
            reply->deleteLater();
            done(std::move(result));
        });
    };
}

Transport make_network_transport(QObject* owner, TransportLimits limits) {
    auto auth = make_authorized_transport(owner, limits);
    return [auth](const QUrl& url, FetchDone done) { auth(url, {}, std::move(done)); };
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
    auto& lifecycle = AppLifecycle::instance();
    paused_ = lifecycle.isSuspended();
    connect(&lifecycle, &AppLifecycle::suspended, this, &PolledSource::pause);
    connect(&lifecycle, &AppLifecycle::resumed, this, &PolledSource::resume);
    if (auto* info = QNetworkInformation::instance()) {
        connect(info, &QNetworkInformation::reachabilityChanged, this,
                [this](QNetworkInformation::Reachability reachability) {
            if (reachability == QNetworkInformation::Reachability::Online) networkReturned();
        });
    }
}

QString PolledSource::cachePath() const { return cache_dir() + "/" + cache_name_ + ".cache"; }

void PolledSource::start() {
    loadCache();
    if (fetched_at_.isValid()) {
        const qint64 age = fetched_at_.secsTo(clock_());
        if (age >= 0 && age < policy_.interval.count()) {
            schedule(std::chrono::seconds(std::max<qint64>(1, policy_.interval.count() - age)));
            return;
        }
    }
    fetch();
}

void PolledSource::networkReturned() {
    if (failures_ == 0 || paused_ || in_flight_) return;
    timer_.stop();
    fetch();
}

void PolledSource::refreshNow() {
    timer_.stop();
    fetch();
}

void PolledSource::pause() {
    paused_ = true;
    timer_.stop();
}

void PolledSource::resume() {
    if (!paused_) return;
    paused_ = false;
    if (in_flight_) return; // its answer re-arms the timer
    const auto age = fetched_at_.isValid() ? fetched_at_.secsTo(clock_()) : policy_.interval.count();
    if (failures_ > 0 || !fetched_at_.isValid() || age >= policy_.interval.count()) {
        fetch();
        return;
    }
    schedule(std::chrono::seconds(std::max<qint64>(1, policy_.interval.count() - age)));
}

bool PolledSource::isStale(const QDateTime& now_utc) const {
    if (!fetched_at_.isValid()) return true;
    return fetched_at_.secsTo(now_utc) > policy_.stale_after.count();
}

std::chrono::seconds PolledSource::nextDelay() const {
    if (failures_ == 0) return policy_.interval;
    auto delay = kFirstRetry;
    for (int i = 1; i < failures_ && delay < policy_.max_backoff; ++i) delay *= 2;
    return std::max(hold_, std::min({delay, policy_.max_backoff, policy_.interval}));
}

void PolledSource::fetch() {
    if (in_flight_ || paused_) return;
    // A server that said "not before" gets nothing earlier, whatever asked (refresh, resume).
    if (not_before_.isValid()) {
        const qint64 wait = clock_().secsTo(not_before_);
        if (wait > 0) {
            schedule(std::chrono::seconds(wait));
            return;
        }
    }
    in_flight_ = true;
    QPointer<PolledSource> self(this);
    transport_(url_, [self](FetchResult result) {
        if (self) self->handle(std::move(result));
    });
}

QString PolledSource::rejectReason(const QByteArray& body) const {
    if (!accepts_html_ && looks_like_html(body))
        return "Got a web page instead of data (network sign-in page or proxy?)";
    if (validator_ && !validator_(body)) return "Unexpected data from the server (format changed or cut short)";
    return {};
}

void PolledSource::fail(const QString& error, std::chrono::seconds retry_after, bool rate_limited,
                        bool rejected) {
    ++failures_;
    rejected_ = rejected;
    if (rate_limited) {
        ++rate_limits_;
        // At least the server's wait, and never sooner than the normal interval:
        // a quick retry is what got us limited. Doubles while it keeps refusing.
        auto wait = policy_.interval;
        for (int i = 1; i < rate_limits_ && wait < kMaxRateLimitWait; ++i) wait *= 2;
        hold_ = std::max(retry_after, std::min(wait, kMaxRateLimitWait));
        not_before_ = clock_().addSecs(hold_.count());
        const qint64 minutes = (hold_.count() + 59) / 60;
        last_error_ = error + QString("; next try in %1 min").arg(minutes);
    } else {
        last_error_ = error;
    }
    schedule(nextDelay());
    emit failed(last_error_);
}

void PolledSource::handle(FetchResult result) {
    in_flight_ = false;
    const int status = result.http_status;
    const bool rate_limited = status == 429 || (status == 503 && result.retry_after.count() > 0);
    const bool http_ok = result.error.isEmpty() && status >= 200 && status < 300;
    if (!http_ok || result.body.isEmpty()) {
        const QString error = !result.error.isEmpty() ? result.error
            : status && !http_ok ? describe_http_status(status) : QString("Empty response");
        fail(error, result.retry_after, rate_limited);
        return;
    }
    if (const auto reason = rejectReason(result.body); !reason.isEmpty()) {
        fail(reason, {}, false, true);
        return;
    }
    failures_ = 0;
    rejected_ = false;
    rate_limits_ = 0;
    hold_ = std::chrono::seconds{0};
    not_before_ = {};
    last_error_.clear();
    body_ = std::move(result.body);
    fetched_at_ = clock_();
    saveCache();
    schedule(policy_.interval);
    emit updated(false);
}

void PolledSource::schedule(std::chrono::seconds delay) {
    if (paused_) return;
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
