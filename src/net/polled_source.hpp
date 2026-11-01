#pragma once
// Shared polling/caching for every live data widget. One PolledSource per
// remote document: fetch on a fixed interval, back off on failure, keep the
// last good body on disk so a restart shows cached data (marked with its age)
// before the network answers. Network access is injected so tests run offline.
#include <QByteArray>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <chrono>
#include <functional>
#include <memory>

class QNetworkReply;

namespace hamclock {

struct FetchResult {
    int http_status = 0;    // 0 when no HTTP response (DNS, timeout, offline)
    QByteArray body;
    QString error;          // empty on success; short user-facing text otherwise
    std::chrono::seconds retry_after{0}; // server's Retry-After on 429/503; 0 when absent
};
using FetchDone = std::function<void(FetchResult)>;
// Performs one GET and calls done exactly once on the GUI thread.
using Transport = std::function<void(const QUrl&, FetchDone)>;

// Limits on one GET. inactivity_ms stops a silent server; total_ms stops a slow
// trickle that keeps sending a byte now and then; max_bytes aborts the download
// as soon as it grows past the limit instead of buffering it all.
struct TransportLimits {
    int inactivity_ms = 20000;
    int total_ms = 120000;
    qint64 max_bytes = 4 * 1024 * 1024;
};

// Real transport over QNetworkAccessManager: honest User-Agent, timeouts,
// redirects limited to the same scheme. Lives as long as the returned callable.
Transport make_network_transport(QObject* owner, TransportLimits limits = {});
// Same, but sends `Authorization: Bearer <token>` when the token is non-empty
// (hams.at API key). Optional: widgets fall back to the plain transport.
using AuthTransport = std::function<void(const QUrl&, const QByteArray& bearer, FetchDone)>;
AuthTransport make_authorized_transport(QObject* owner, TransportLimits limits = {});
QByteArray hamclock_user_agent();

// Short user-facing text for an HTTP failure status, always starting "HTTP <code>"
// (widgets match on that prefix, e.g. hams.at's 401 fallback).
QString describe_http_status(int http_status);
// User-facing text for a transport failure without a usable HTTP status
// (QNetworkReply::NetworkError value). Never contains the URL, which may hold
// the user's callsign or query.
QString describe_network_error(int network_error);
// Turns a finished reply into what the real transport hands its caller: friendly
// error text (never Qt's errorString(), which repeats the URL), HTTP status,
// Retry-After and the body capped at max_bytes. abort_reason, when non-empty, is
// why the transport aborted the reply itself (deadline, size limit).
FetchResult fetch_result_from_reply(QNetworkReply& reply, const QString& abort_reason, qint64 max_bytes);
// Retry-After header: delta-seconds or an HTTP-date. Zero when absent or invalid;
// capped at one day so a bad header cannot silence a source for good.
std::chrono::seconds parse_retry_after(const QByteArray& value, const QDateTime& now_utc);
// True for an HTML page (captive portal sign-in, proxy or CDN error page) where a
// data document was expected.
bool looks_like_html(const QByteArray& body);

struct PollPolicy {
    std::chrono::seconds interval{900};      // normal refresh period
    std::chrono::seconds stale_after{3600};  // older data is shown as stale
    std::chrono::seconds max_backoff{3600};  // cap for failure retry delay
};

class PolledSource : public QObject {
    Q_OBJECT
public:
    // cache_name: file-safe identifier ([a-z0-9-]+), stored under the app cache dir.
    PolledSource(QString cache_name, QUrl url, PollPolicy policy, Transport transport,
                 QObject* parent = nullptr);

    // Load the cache, then fetch at once unless the cached copy is younger than the
    // interval (an app restart must not re-download what it already has; Launch
    // Library allows 15 requests per hour and CelesTrak blocks repeat downloads).
    void start();
    void refreshNow();
    // Stop polling while the app is suspended (iPad background); resume fetches
    // at once if the data is older than the interval, else waits out the rest.
    void pause();
    void resume();
    bool isPaused() const { return paused_; }
    // The device came back online: a failing source retries now (still honouring a
    // server's Retry-After) instead of waiting out its backoff. Wired to
    // QNetworkInformation when a backend is loaded.
    void networkReturned();
    const QByteArray& body() const { return body_; }
    QDateTime fetchedAt() const { return fetched_at_; } // UTC; invalid if never
    bool isStale(const QDateTime& now_utc) const;
    QString lastError() const { return last_error_; }
    int consecutiveFailures() const { return failures_; }
    // True when the last answer arrived but was refused (HTML page, failed validator).
    bool lastBodyRejected() const { return rejected_; }
    std::chrono::seconds nextDelay() const;           // for tests/diagnostics
    QString cachePath() const;

    // Earliest time the server allows another request after a 429/503 (invalid when none).
    QDateTime retryNotBefore() const { return not_before_; }
    // Bodies failing the check are treated as a failed fetch: the last good body and
    // its cache stay, so a changed schema or garbage never replaces good data.
    void setValidator(std::function<bool(const QByteArray&)> validator) { validator_ = std::move(validator); }
    // HTML bodies are rejected unless the source really serves HTML (NHC advisories).
    void setAcceptsHtml(bool accepts) { accepts_html_ = accepts; }

    // Clock injection for tests; defaults to QDateTime::currentDateTimeUtc.
    void setClock(std::function<QDateTime()> clock) { clock_ = std::move(clock); }

signals:
    // New good body (fresh from network, or loaded from cache at start).
    void updated(bool from_cache);
    void failed(const QString& error);

private:
    void fetch();
    void handle(FetchResult result);
    void fail(const QString& error, std::chrono::seconds retry_after, bool rate_limited, bool rejected = false);
    QString rejectReason(const QByteArray& body) const;
    void schedule(std::chrono::seconds delay);
    void loadCache();
    void saveCache();

    QString cache_name_;
    QUrl url_;
    PollPolicy policy_;
    Transport transport_;
    QTimer timer_;
    QByteArray body_;
    QDateTime fetched_at_;
    QString last_error_;
    int failures_ = 0;
    int rate_limits_ = 0;          // consecutive 429/503-with-Retry-After answers
    std::chrono::seconds hold_{0}; // rate-limit wait that overrides the normal backoff
    QDateTime not_before_;
    bool in_flight_ = false;
    bool paused_ = false;
    bool accepts_html_ = false;
    bool rejected_ = false;
    std::function<bool(const QByteArray&)> validator_;
    std::function<QDateTime()> clock_;
};

} // namespace hamclock
