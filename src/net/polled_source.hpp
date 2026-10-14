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

namespace hamclock {

struct FetchResult {
    int http_status = 0;    // 0 when no HTTP response (DNS, timeout, offline)
    QByteArray body;
    QString error;          // empty on success
};
using FetchDone = std::function<void(FetchResult)>;
// Performs one GET and calls done exactly once on the GUI thread.
using Transport = std::function<void(const QUrl&, FetchDone)>;

// Real transport over QNetworkAccessManager: honest User-Agent, timeout,
// redirects limited to the same scheme. Lives as long as the returned callable.
Transport make_network_transport(QObject* owner);
QByteArray hamclock_user_agent();

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

    void start();           // load cache, then fetch immediately
    void refreshNow();
    // Stop polling while the app is suspended (iPad background); resume fetches
    // at once if the data is older than the interval, else waits out the rest.
    void pause();
    void resume();
    bool isPaused() const { return paused_; }
    const QByteArray& body() const { return body_; }
    QDateTime fetchedAt() const { return fetched_at_; } // UTC; invalid if never
    bool isStale(const QDateTime& now_utc) const;
    QString lastError() const { return last_error_; }
    int consecutiveFailures() const { return failures_; }
    std::chrono::seconds nextDelay() const;           // for tests/diagnostics
    QString cachePath() const;

    // Clock injection for tests; defaults to QDateTime::currentDateTimeUtc.
    void setClock(std::function<QDateTime()> clock) { clock_ = std::move(clock); }

signals:
    // New good body (fresh from network, or loaded from cache at start).
    void updated(bool from_cache);
    void failed(const QString& error);

private:
    void fetch();
    void handle(FetchResult result);
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
    bool in_flight_ = false;
    bool paused_ = false;
    std::function<QDateTime()> clock_;
};

} // namespace hamclock
