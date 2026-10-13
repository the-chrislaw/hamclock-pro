#pragma once
// Parsers and URL builders for the QRZ.com XML and HamQTH XML callsign
// lookups. Pure functions (no network, no settings) so they test offline.
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <optional>

namespace hamclock::lookup {

struct Record {
    QString call, name, qth, country, grid;
};

enum class Outcome { Ok, NotFound, SessionExpired, LoginFailed, Error };

// One reply, either a login reply (session set), a lookup reply (record set)
// or an error. message is a short user-facing text; it never contains the
// request URL or the password.
struct Reply {
    Outcome outcome = Outcome::Error;
    QString session;
    std::optional<Record> record;
    QString message;
};

Reply parse_qrz(const QByteArray& body);
Reply parse_hamqth(const QByteArray& body);

// Callsign as typed -> canonical upper case, or empty when it is not plausible.
QString normalize_call(const QString& text);

QUrl qrz_login_url(const QString& user, const QString& password);
QUrl qrz_lookup_url(const QString& session, const QString& call);
QUrl hamqth_login_url(const QString& user, const QString& password);
QUrl hamqth_lookup_url(const QString& session, const QString& call);

} // namespace hamclock::lookup
