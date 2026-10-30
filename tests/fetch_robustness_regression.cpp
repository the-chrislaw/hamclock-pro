// Data robustness: every failure mode a public feed can show (rate limit with
// Retry-After, captive-portal HTML, schema change, slow trickle, huge body, DNS
// failure, restart with a fresh cache, network coming back) must leave the last
// good data in place, say what went wrong in plain words and never hammer the
// server. The real transport is exercised against a local HTTP server.
#include "../src/net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimeZone>
#include <QTimer>
#include <functional>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
void spin(QCoreApplication& app) { for (int i = 0; i < 10; ++i) app.processEvents(); }

struct FakeNet {
    hamclock::FetchResult next{200, "[1]", {}};
    int calls = 0;
    hamclock::Transport transport() {
        return [this](const QUrl&, hamclock::FetchDone done) {
            ++calls;
            auto result = next;
            QTimer::singleShot(0, [done, result] { done(result); });
        };
    }
};

// One-shot HTTP/1.1 server: each connection gets `respond(socket)`.
class LocalServer {
public:
    explicit LocalServer(std::function<void(QTcpSocket*)> respond) : respond_(std::move(respond)) {
        server_.listen(QHostAddress::LocalHost);
        QObject::connect(&server_, &QTcpServer::newConnection, [this] {
            while (auto* socket = server_.nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                    if (socket->property("answered").toBool()) return;
                    if (!socket->readAll().contains("\r\n\r\n")) return;
                    socket->setProperty("answered", true);
                    respond_(socket);
                });
            }
        });
    }
    QUrl url() const { return QUrl(QString("http://127.0.0.1:%1/feed.json").arg(server_.serverPort())); }
private:
    QTcpServer server_;
    std::function<void(QTcpSocket*)> respond_;
};

hamclock::FetchResult fetch_once(QCoreApplication& app, const hamclock::Transport& transport, const QUrl& url,
                                 int wait_ms = 10000) {
    hamclock::FetchResult out;
    bool done = false;
    transport(url, [&](hamclock::FetchResult r) { out = std::move(r); done = true; });
    QElapsedTimer t; t.start();
    while (!done && t.elapsed() < wait_ms) app.processEvents(QEventLoop::AllEvents, 20);
    if (!done) out.error = "TEST: transport never answered";
    return out;
}

void reply(QTcpSocket* s, const QByteArray& status, const QByteArray& headers, const QByteArray& body) {
    s->write("HTTP/1.1 " + status + "\r\n" + headers + "Content-Length: " + QByteArray::number(body.size()) +
             "\r\nConnection: close\r\n\r\n" + body);
    s->flush();
    s->disconnectFromHost();
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("hamclock-pro-fetch-test");
    QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).removeRecursively();
    // On the Mac PolledSource follows XDG_CACHE_HOME (set per test by CMake) instead.
    if (const QString xdg = qEnvironmentVariable("XDG_CACHE_HOME"); QDir::isAbsolutePath(xdg))
        QDir(xdg + "/" + QCoreApplication::applicationName() + "/sources").removeRecursively();
    // Only localhost is used; skip the system proxy lookup (Linux libproxy leaks
    // a few bytes inside glib, which fails the ASan job).
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    using namespace hamclock;
    using namespace std::chrono_literals;
    const auto t0 = QDateTime(QDate(2026, 10, 29), QTime(12, 0), QTimeZone::utc());

    // Pure helpers.
    check(parse_retry_after("120", t0) == 120s, "Retry-After seconds");
    check(parse_retry_after(" 30 ", t0) == 30s, "Retry-After seconds with spaces");
    check(parse_retry_after("Thu, 29 Oct 2026 12:10:00 GMT", t0) == 600s, "Retry-After HTTP-date");
    check(parse_retry_after("Thu, 29 Oct 2026 11:00:00 GMT", t0) == 0s, "Retry-After in the past is zero");
    check(parse_retry_after("soon", t0) == 0s && parse_retry_after("", t0) == 0s && parse_retry_after("-5", t0) == 0s,
          "invalid Retry-After is zero");
    check(parse_retry_after("999999999", t0) == 86400s, "Retry-After capped at a day");
    check(describe_http_status(429).startsWith("HTTP 429") && describe_http_status(429).contains("rate limit"),
          "429 says rate limited");
    check(describe_http_status(401).startsWith("HTTP 401"), "401 keeps the HTTP 401 prefix (hams.at fallback)");
    check(describe_http_status(503).startsWith("HTTP 503") && describe_http_status(503).contains("server"),
          "5xx says server error");
    check(describe_http_status(404).contains("not found"), "404 says not found");
    check(describe_network_error(QNetworkReply::HostNotFoundError).contains("DNS"), "DNS failure explained");
    check(describe_network_error(QNetworkReply::TimeoutError).contains("Timed out"), "timeout explained");
    check(describe_network_error(QNetworkReply::ConnectionRefusedError).contains("refused"), "refused explained");
    check(describe_network_error(QNetworkReply::SslHandshakeFailedError).contains("Secure connection"), "TLS failure explained");
    check(!describe_network_error(QNetworkReply::UnknownNetworkError).isEmpty(), "unknown error has text");
    check(looks_like_html("<!DOCTYPE html><html><body>Sign in to Wi-Fi</body></html>") &&
          looks_like_html("\n  <HTML><head>") && looks_like_html("\xEF\xBB\xBF<html>") &&
          looks_like_html("<head><meta http-equiv=refresh>") &&
          looks_like_html("\t<!-- copied by nhcWeb -->\n\n<!DOCTYPE html>\n<html lang=\"en\">"),
          "captive portal pages detected");
    check(!looks_like_html("[{\"Kp\":2}]") && !looks_like_html("<?xml version=\"1.0\"?><solar/>") &&
          !looks_like_html("latitude,longitude\n1,2") && !looks_like_html("\xFF\xD8\xFF\xE0JFIF"),
          "JSON, XML, CSV and JPEG are not HTML");

    FakeNet net;
    const PollPolicy policy{300s, 1200s, 1800s};

    // 429 with Retry-After: no request before the server allows it, even on refresh/resume.
    {
        PolledSource s("fr-rate", QUrl("https://x.test/rate"), policy, net.transport());
        QDateTime now = t0;
        s.setClock([&] { return now; });
        s.start(); spin(app);
        check(s.body() == "[1]", "first body");
        net.next = {429, "{\"detail\":\"throttled\"}", {}, 701s};
        s.refreshNow(); spin(app);
        check(s.body() == "[1]", "429 keeps the good body");
        check(s.nextDelay() >= 701s, "429 waits at least Retry-After");
        check(s.lastError().startsWith("HTTP 429") && s.lastError().contains("12 min"),
              "429 text says rate limited and when it retries");
        check(s.retryNotBefore() == t0.addSecs(701), "not-before recorded");
        const int calls = net.calls;
        net.next = {200, "[2]", {}};
        s.refreshNow(); spin(app);
        check(net.calls == calls, "refresh during Retry-After does not hit the server");
        s.pause(); s.resume(); spin(app);
        check(net.calls == calls, "resume during Retry-After does not hit the server");
        now = t0.addSecs(702);
        s.refreshNow(); spin(app);
        check(net.calls == calls + 1 && s.body() == "[2]" && s.lastError().isEmpty(), "fetches again after Retry-After");
        check(s.nextDelay() == 300s && !s.retryNotBefore().isValid(), "success clears the rate limit");
    }
    // 429 without Retry-After (PSKReporter): back off by at least the interval, doubling.
    {
        PolledSource s("fr-rate2", QUrl("https://x.test/rate2"), policy, net.transport());
        s.setClock([&] { return t0; });
        net.next = {429, "{}", {}};
        s.start(); spin(app);
        check(s.nextDelay() >= 300s, "429 without Retry-After waits at least one interval");
        const auto first = s.nextDelay();
        s.setClock([&] { return t0.addSecs(7200); });
        s.refreshNow(); spin(app);
        check(s.nextDelay() >= first * 2 || s.nextDelay() >= 6h, "repeated 429 doubles the wait");
        check(s.lastError().startsWith("HTTP 429"), "429 without body text still explained");
    }
    // Captive portal: a 200 HTML page never replaces data, never resets age.
    {
        net.next = {200, "[1]", {}};
        PolledSource s("fr-portal", QUrl("https://x.test/portal"), policy, net.transport());
        s.setClock([&] { return t0; });
        s.start(); spin(app);
        net.next = {200, "<!DOCTYPE html><html><title>Hotel Wi-Fi</title></html>", {}};
        s.setClock([&] { return t0.addSecs(600); });
        int updates = 0;
        QObject::connect(&s, &PolledSource::updated, [&] { ++updates; });
        s.refreshNow(); spin(app);
        check(updates == 0 && s.body() == "[1]" && s.fetchedAt() == t0, "HTML portal page rejected");
        check(s.lastError().contains("web page"), "portal error explained");
        check(s.consecutiveFailures() == 1, "portal counts as a failure (backs off)");
    }
    {
        PolledSource s("fr-html-ok", QUrl("https://x.test/html"), policy, net.transport());
        s.setAcceptsHtml(true);
        net.next = {200, "<html><pre>ADVISORY</pre></html>", {}};
        s.start(); spin(app);
        check(s.body().contains("ADVISORY"), "HTML accepted when the source serves HTML");
    }
    // Changed schema / truncated JSON: the validator keeps the old body and its cache.
    {
        QFile::remove(PolledSource("fr-schema", QUrl("https://x.test/s"), policy, net.transport()).cachePath());
        net.next = {200, "[{\"Kp\":2}]", {}};
        PolledSource s("fr-schema", QUrl("https://x.test/s"), policy, net.transport());
        s.setClock([&] { return t0; });
        s.setValidator([](const QByteArray& b) { return b.startsWith("[{") && b.endsWith("]"); });
        s.start(); spin(app);
        net.next = {200, "[{\"Kp\":2", {}};
        s.setClock([&] { return t0.addSecs(400); });
        s.refreshNow(); spin(app);
        check(s.body() == "[{\"Kp\":2}]" && s.fetchedAt() == t0, "truncated body rejected by validator");
        check(s.lastError().contains("Unexpected"), "validator failure explained");
        QFile cache(s.cachePath());
        check(cache.open(QIODevice::ReadOnly) && cache.readAll().contains("[{\"Kp\":2}]"), "cache keeps the good body");
    }
    // Restart with a fresh cache: show it and wait out the interval instead of re-downloading.
    {
        net.next = {200, "[7]", {}};
        QFile::remove(PolledSource("fr-restart", QUrl("https://x.test/r"), policy, net.transport()).cachePath());
        {
            PolledSource s("fr-restart", QUrl("https://x.test/r"), policy, net.transport());
            s.setClock([&] { return t0; });
            s.start(); spin(app);
        }
        const int calls = net.calls;
        {
            PolledSource s("fr-restart", QUrl("https://x.test/r"), policy, net.transport());
            s.setClock([&] { return t0.addSecs(60); });
            bool cached = false;
            QObject::connect(&s, &PolledSource::updated, [&](bool c) { cached = c; });
            s.start(); spin(app);
            check(cached && s.body() == "[7]" && net.calls == calls, "fresh cache shown without a new request");
        }
        {
            PolledSource s("fr-restart", QUrl("https://x.test/r"), policy, net.transport());
            s.setClock([&] { return t0.addSecs(301); });
            s.start(); spin(app);
            check(net.calls == calls + 1, "old cache fetched at once on restart");
        }
        {
            PolledSource s("fr-restart", QUrl("https://x.test/r"), policy, net.transport());
            s.setClock([&] { return t0.addSecs(-3600); }); // clock went backwards
            const int before = net.calls;
            s.start(); spin(app);
            check(net.calls == before + 1, "cache from the future is not trusted");
        }
    }
    // Network returns: a failing source retries at once instead of waiting out its backoff.
    {
        net.next = {0, {}, "No network connection"};
        PolledSource s("fr-online", QUrl("https://x.test/o"), policy, net.transport());
        s.start(); spin(app);
        const int calls = net.calls;
        net.next = {200, "[3]", {}};
        s.networkReturned(); spin(app);
        check(net.calls == calls + 1 && s.body() == "[3]", "network return refetches a failing source");
        s.networkReturned(); spin(app);
        check(net.calls == calls + 1, "network return leaves a healthy source alone");
    }

    // Real transport against a local server.
    {
        TransportLimits limits;
        limits.total_ms = 1500;
        limits.max_bytes = 64 * 1024;
        QObject owner;
        const auto transport = make_network_transport(&owner, limits);

        LocalServer rate([](QTcpSocket* s) { reply(s, "429 Too Many Requests", "Retry-After: 583\r\n", "{\"detail\":\"x\"}"); });
        auto r = fetch_once(app, transport, rate.url());
        check(r.http_status == 429 && r.retry_after == 583s, "transport reports 429 and Retry-After");
        check(r.error.startsWith("HTTP 429") && !r.error.contains("127.0.0.1"), "transport error is friendly, no URL");

        LocalServer missing([](QTcpSocket* s) { reply(s, "404 Not Found", "", "nope"); });
        r = fetch_once(app, transport, missing.url());
        check(r.http_status == 404 && r.error.startsWith("HTTP 404") && !r.error.contains("feed.json"), "404 friendly");

        LocalServer ok([](QTcpSocket* s) { reply(s, "200 OK", "Content-Type: application/json\r\n", "[1,2,3]"); });
        r = fetch_once(app, transport, ok.url());
        check(r.http_status == 200 && r.body == "[1,2,3]" && r.error.isEmpty(), "transport success");

        LocalServer huge([](QTcpSocket* s) {
            const QByteArray body(200 * 1024, 'x');
            reply(s, "200 OK", "", body);
        });
        r = fetch_once(app, transport, huge.url());
        check(r.body.isEmpty() && r.error.contains("too large"), "huge body refused");

        // Slow trickle: one byte every 200 ms never trips the inactivity timeout,
        // so only the total deadline stops it.
        LocalServer trickle([](QTcpSocket* s) {
            s->write("HTTP/1.1 200 OK\r\nContent-Length: 1000\r\nConnection: close\r\n\r\n");
            auto* t = new QTimer(s);
            QObject::connect(t, &QTimer::timeout, s, [s] { s->write("x"); s->flush(); });
            t->start(200);
        });
        QElapsedTimer clock; clock.start();
        r = fetch_once(app, transport, trickle.url(), 8000);
        check(r.error.contains("Timed out") && r.body.isEmpty(), "slow trickle stopped by the total deadline");
        check(clock.elapsed() < 5000, "deadline is honoured promptly");

        LocalServer hangup([](QTcpSocket* s) { s->abort(); });
        r = fetch_once(app, transport, hangup.url());
        check(!r.error.isEmpty() && r.body.isEmpty(), "connection dropped reported");

        r = fetch_once(app, transport, QUrl("http://does-not-exist.invalid/feed.json"));
        check(r.http_status == 0 && r.error.contains("DNS") && !r.error.contains("invalid"), "DNS failure friendly");
    }

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
