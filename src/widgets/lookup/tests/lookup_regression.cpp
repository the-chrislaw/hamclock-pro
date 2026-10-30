// Callsign lookup: QRZ and HamQTH parsers and the session-handling model.
// Fixtures follow the services' documented XML formats (QRZ XML spec 1.34,
// HamQTH XML 2.8 docs); they are NOT live captures because a real reply needs
// a personal account. Offline: the transport is a fake.
#include "../lookup_model.hpp"
#include "../lookup_parse.hpp"
#include "../../../ui/app_settings.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
QByteArray g_dir;
QByteArray fx(const char* name) {
    QFile f(QDir(g_dir).filePath(name));
    check(f.open(QIODevice::ReadOnly), name);
    return f.readAll();
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    using lookup::Outcome;
    if (argc < 2) { std::cerr << "usage: test FIXTURE_DIR\n"; return 2; }
    g_dir = argv[1];

    auto login = lookup::parse_qrz(fx("qrz_login.xml"));
    check(login.outcome == Outcome::Ok && login.session.startsWith("2331uf") && !login.record, "qrz login");
    auto q = lookup::parse_qrz(fx("qrz_callsign.xml"));
    check(q.outcome == Outcome::Ok && q.record && q.record->call == "W1AW" && q.record->name == "Hiram Percy Maxim" &&
          q.record->qth == "Newington" && q.record->country == "United States" && q.record->grid == "FN31PR", "qrz record");
    check(lookup::parse_qrz(fx("qrz_notfound.xml")).outcome == Outcome::NotFound, "qrz not found");
    check(lookup::parse_qrz(fx("qrz_expired.xml")).outcome == Outcome::SessionExpired, "qrz expired");
    check(lookup::parse_qrz(fx("qrz_badlogin.xml")).outcome == Outcome::LoginFailed, "qrz bad login");
    check(lookup::parse_qrz(fx("qrz_login.xml")).subscription == "Wed Jan 1 12:34:03 2025", "qrz subscription expiry kept");
    {
        const auto nonsub = lookup::parse_qrz(fx("qrz_login_nonsub.xml"));
        check(nonsub.outcome == Outcome::Ok && nonsub.session.startsWith("7a1b2c") && nonsub.subscription == "non-subscriber" &&
                  !lookup::has_xml_subscription(nonsub.subscription),
              "qrz non-subscriber login is a session without a subscription");
        check(lookup::has_xml_subscription("Wed Jan 1 12:34:03 2025") && lookup::has_xml_subscription(""),
              "an expiry date (or no field, HamQTH) counts as subscribed");
    }
    // QRZ: "Connection refused" means no login will succeed for at least 24 hours (spec 1.34).
    check(lookup::parse_qrz(fx("qrz_refused.xml")).outcome == Outcome::Blocked, "qrz connection refused is a block, not a bad password");

    login = lookup::parse_hamqth(fx("hamqth_login.xml"));
    check(login.outcome == Outcome::Ok && login.session.startsWith("09b0ae"), "hamqth login");
    q = lookup::parse_hamqth(fx("hamqth_callsign.xml"));
    check(q.outcome == Outcome::Ok && q.record && q.record->call == "OK2CQR" && q.record->name == "Petr Hlozek" &&
          q.record->grid == "JO70GA" && q.record->country == "Czech Republic", "hamqth record");
    check(lookup::parse_hamqth(fx("hamqth_notfound.xml")).outcome == Outcome::NotFound, "hamqth not found");
    check(lookup::parse_hamqth(fx("hamqth_badlogin.xml")).outcome == Outcome::LoginFailed, "hamqth bad login");
    for (const char* bad : {"", "<x/>", "<QRZDatabase><Session>", "not xml"})
        check(lookup::parse_qrz(bad).outcome == Outcome::Error, "malformed rejected");

    check(lookup::normalize_call(" w1aw ") == "W1AW" && lookup::normalize_call("dl1abc/p") == "DL1ABC/P" &&
          lookup::normalize_call("3DA0XX") == "3DA0XX" && lookup::normalize_call("").isEmpty() &&
          lookup::normalize_call("a b").isEmpty() && lookup::normalize_call("hello").isEmpty() &&
          lookup::normalize_call("W1AW&x=1").isEmpty(), "callsign normalisation");
    const auto url = lookup::qrz_login_url("me", "p&ss=w d");
    check(QString::fromLatin1(url.toEncoded()).contains("password=p%26ss%3Dw%20d") && url.host() == "xmldata.qrz.com" && url.scheme() == "https",
          "password percent-encoded, https");

    // Path from DE to a grid centre (DE at 0N 0E; JJ00 centre is 0.5N 1E).
    check(LookupModel::pathText(0, 0, "").isEmpty() && LookupModel::pathText(0, 0, "ZZ99").isEmpty() &&
              LookupModel::pathText(0, 0, "FN3").isEmpty(),
          "no path for empty or malformed grid");
    {
        const QString p = LookupModel::pathText(41.7, -72.7, "FN31pr");
        check(p.contains(" km") && p.endsWith("SP"), "path text shape");
        const QString east = LookupModel::pathText(0, 0, "JJ00");  // centre 0.5N 1E: ~124 km, bearing ~63
        check(east.startsWith("124 km") && east.contains("63°"), "path distance and bearing");
    }

    // Model with a fake transport.
    QTemporaryDir dir;
    AppSettings settings(dir.filePath("station.conf"), dir.path());
    WidgetContext context;
    context.settings = &settings;
    QStringList urls;
    QString mode = "ok";
    context.transport = [&](const QUrl& u, FetchDone done) {
        urls << u.toString();
        FetchResult r;
        r.http_status = 200;
        const QString s = u.toString();
        if (mode == "down") { r.http_status = 0; r.error = "offline"; }
        else if (mode == "busy") { r.http_status = 503; r.error = "HTTP 503"; r.body = "<html>Service Unavailable</html>"; }
        else if (s.contains("hamqth.com")) r.body = fx(mode == "badlogin" ? "hamqth_badlogin.xml"
                                                       : s.contains("p=") ? "hamqth_login.xml" : "hamqth_callsign.xml");
        else if (s.contains("password=")) r.body = fx(mode == "badlogin" ? "qrz_badlogin.xml" : mode == "refused" ? "qrz_refused.xml"
                                                       : mode == "nonsub" ? "qrz_login_nonsub.xml" : "qrz_login.xml");
        else if (mode == "expire-once" && !s.contains("W1AW-RETRY")) { mode = "ok"; r.body = fx("qrz_expired.xml"); }
        else if (s.contains("N0XYZ")) r.body = fx("qrz_notfound.xml");
        else r.body = fx("qrz_callsign.xml");
        QTimer::singleShot(0, [done, r] { done(r); });
    };
    LookupModel model(context);
    auto settle = [&] { for (int i = 0; i < 20; ++i) QCoreApplication::processEvents(); };
    model.lookup("W1AW");
    check(model.state() == "unconfigured" && urls.isEmpty(), "no account: nothing sent");

    check(settings.saveAccount("qrz", "me", "secret").isEmpty(), "account saved");
    check(model.configured(), "configured");
    model.lookup("not a call");
    check(model.message() == "Not a callsign" && urls.isEmpty(), "bad input not sent");

    model.lookup("w1aw");
    check(model.busy() && model.state() == "loading", "busy while asking");
    settle();
    check(!model.busy() && model.call() == "W1AW" && model.name() == "Hiram Percy Maxim" &&
          model.location() == "Newington, United States" && model.grid() == "FN31PR" && model.state() == "live",
          "result shown");
    check(urls.size() == 2 && urls[0].contains("password=") && urls[1].contains("s=2331uf"), "login then query");
    model.lookup("W1AW");
    check(urls.size() == 2, "second lookup cached");

    mode = "expire-once";
    model.lookup("N0XYZ");
    settle();
    check(urls.size() == 5, "expired session: one re-login and a retry");
    check(model.call().isEmpty() && model.message() == "N0XYZ not found" && model.state() == "error", "not found state");

    settings.saveAccount("qrz", "me", "wrong");
    mode = "badlogin";
    model.lookup("W1AW");
    settle();
    check(model.state() == "error" && model.message().contains("check user and password"), "bad login reported");
    check(!model.statusText().contains("wrong") && !model.message().contains("wrong") && !model.message().contains("secret"),
          "password never shown");

    mode = "down";
    settings.saveAccount("qrz", "me", "pw2");
    model.lookup("DL1ABC");
    settle();
    check(model.state() == "error" && model.message() == "Lookup service unreachable", "offline reported");

    // Server busy: an HTTP 5xx with an HTML body is not a bad password.
    mode = "busy";
    settings.saveAccount("qrz", "me", "pw3");
    model.lookup("DL1ABC");
    settle();
    check(model.state() == "error" && model.message().contains("busy") && !model.message().contains("password"),
          "server busy reported as busy");

    // QRZ's 24-hour block.
    mode = "refused";
    settings.saveAccount("qrz", "me", "pw4");
    model.lookup("DL1ABC");
    settle();
    check(model.state() == "error" && model.message().contains("24 hours"), "qrz block explained");

    // Settings > Accounts: each lookup service is checked on its own (login only), and the
    // answer lands in checks()[service] without touching the lookup pane.
    const auto check_of = [&](const char* service) { return model.checks().value(service).toMap(); };
    check(check_of("qrz")["state"] == "blocked", "a failed lookup login also updates the account check");
    urls.clear();
    model.checkLogin("qrz");
    check(check_of("qrz")["state"] == "checking" && !model.busy(), "check: checking, pane not busy");
    settle();
    check(check_of("qrz")["message"].toString().contains("24 hours") && urls.size() == 1 && urls[0].contains("password="),
          "check: login only");
    mode = "ok";
    check(settings.signIn("qrz", "me", "pw5").isEmpty(), "sign in to QRZ");
    check(model.checks().isEmpty(), "a changed password drops the old check");
    model.checkLogin("qrz");
    settle();
    check(check_of("qrz")["state"] == "ok" && check_of("qrz")["message"].toString().contains("Wed Jan 1 12:34:03 2025") &&
              !check_of("qrz")["message"].toString().contains("pw5"),
          "check: subscribed account reported with expiry");
    check(model.call().isEmpty(), "check: no lookup result shown");
    urls.clear();
    model.lookup("K1ABC");
    settle();
    check(urls.size() == 1 && !urls[0].contains("password="), "a good check of the used account saves the lookup a login");
    mode = "nonsub";
    check(settings.signIn("qrz", "me", "pw6").isEmpty(), "sign in to QRZ without XML");
    model.checkLogin("qrz");
    settle();
    check(check_of("qrz")["state"] == "limited" && check_of("qrz")["message"].toString().contains("no XML") &&
              model.subscriptionWarning(),
          "check: non-subscriber warned");
    model.lookup("W1AW");
    settle();
    check(model.call() == "W1AW" && model.statusText().contains("limited"), "non-subscriber lookups still run, marked limited");
    mode = "badlogin";
    check(settings.signIn("qrz", "me", "pw7").isEmpty(), "sign in with a wrong password");
    model.checkLogin("qrz");
    settle();
    check(check_of("qrz")["state"] == "rejected" && check_of("qrz")["message"].toString().contains("check user and password"),
          "check: bad login reported");

    // HamQTH (free) signed in next to QRZ.com: lookups use QRZ.com, both can be checked.
    mode = "ok";
    check(settings.signIn("hamqth", "W1AW", "pw8").isEmpty() && settings.lookupService() == "qrz", "QRZ.com preferred");
    model.checkLogin("hamqth");
    settle();
    check(check_of("hamqth")["state"] == "ok" && check_of("hamqth")["message"].toString().contains("HamQTH accepted") &&
              check_of("qrz")["state"] == "rejected",
          "check: hamqth checked on its own, qrz result kept");
    check(settings.signOut("qrz").isEmpty() && settings.lookupService() == "hamqth", "signing out of QRZ.com falls back to HamQTH");
    model.lookup("OK2CQR");
    settle();
    check(model.call() == "OK2CQR" && urls.last().contains("prg=hamclock-pro") && urls.last().contains("id=09b0ae"),
          "hamqth lookup uses the session id and names the program");
    mode = "badlogin";
    check(settings.signIn("hamqth", "W1AW", "pw9").isEmpty(), "hamqth wrong password saved");
    model.checkLogin("hamqth");
    settle();
    check(check_of("hamqth")["state"] == "rejected", "check: hamqth bad login");
    check(settings.signOut("hamqth").isEmpty(), "sign out of hamqth");
    urls.clear();
    model.checkLogin("hamqth");
    check(urls.isEmpty() && model.state() == "unconfigured", "check without an account does nothing");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
