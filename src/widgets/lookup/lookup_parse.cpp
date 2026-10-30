#include "lookup_parse.hpp"
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace hamclock::lookup {
namespace {
const QString kAgent = "hamclock-pro";

QString clip(QString s) { return s.simplified().left(80); }

// Flat element map of the first <Callsign>/<search> block plus session fields.
struct Doc {
    QString session, error, subscription, call, name, nick, city, country, grid;
    bool has_record = false;
    bool valid = false;
};

Doc read(const QByteArray& body, const QString& root, const QString& record_tag, const QString& key_tag) {
    Doc d;
    QXmlStreamReader xml(body);
    QStringList path;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            path << xml.name().toString().toLower();
            if (path.size() == 1) d.valid = path[0] == root;
            if (path.size() == 2 && path[1] == record_tag) d.has_record = true;
        } else if (xml.isEndElement()) {
            if (!path.isEmpty()) path.removeLast();
        } else if (xml.isCharacters() && !xml.isWhitespace() && path.size() == 3) {
            const QString text = xml.text().toString();
            const QString& tag = path[2];
            if (path[1] == key_tag && tag == "key") d.session = text.trimmed();
            else if (path[1] == key_tag && tag == "session_id") d.session = text.trimmed();
            else if (path[1] == "session" && tag == "error") d.error = clip(text);
            else if (path[1] == "session" && tag == "subexp") d.subscription = clip(text);
            else if (path[1] == record_tag) {
                if (tag == "call" || tag == "callsign") d.call = clip(text);
                else if (tag == "fname" || tag == "adr_name") d.name = clip(text);
                else if (tag == "name") d.name = d.name.isEmpty() ? clip(text) : d.name + " " + clip(text);
                else if (tag == "nick") d.nick = clip(text);
                else if (tag == "addr2" || tag == "qth") d.city = clip(text);
                else if (tag == "country") d.country = clip(text);
                else if (tag == "grid") d.grid = clip(text).toUpper();
            }
        }
    }
    if (xml.hasError()) d.valid = false;
    return d;
}

Reply finish(const Doc& d, bool qrz) {
    Reply r;
    if (!d.valid) { r.message = "Unreadable lookup reply"; return r; }
    if (!d.error.isEmpty()) {
        const QString e = d.error.toLower();
        r.message = d.error;
        if (e.contains("session") || e.contains("invalid session") || e.contains("expired")) r.outcome = Outcome::SessionExpired;
        else if (e.contains("not found")) r.outcome = Outcome::NotFound;
        else if (e.contains("connection refused")) r.outcome = Outcome::Blocked;
        else if (e.contains("password") || e.contains("username") || e.contains("incorrect") || e.contains("refused"))
            r.outcome = Outcome::LoginFailed;
        else r.outcome = Outcome::Error;
        // QRZ puts a session key next to non-fatal messages; keep it.
        if (!d.session.isEmpty() && r.outcome == Outcome::NotFound) r.session = d.session;
        return r;
    }
    if (d.has_record && !d.call.isEmpty()) {
        Record rec{d.call.toUpper(), d.name.isEmpty() ? d.nick : d.name, d.city, d.country, d.grid};
        r.outcome = Outcome::Ok;
        r.record = rec;
        r.session = d.session;
        r.subscription = d.subscription;
        return r;
    }
    if (!d.session.isEmpty()) {
        r.outcome = Outcome::Ok;
        r.session = d.session;
        r.subscription = d.subscription;
        return r;
    }
    (void)qrz;
    r.message = "Unexpected lookup reply";
    return r;
}
} // namespace

Reply parse_qrz(const QByteArray& body) {
    // QRZ's root is <QRZDatabase>; errors live under <Session>.
    return finish(read(body, "qrzdatabase", "callsign", "session"), true);
}

Reply parse_hamqth(const QByteArray& body) {
    return finish(read(body, "hamqth", "search", "session"), false);
}

bool has_xml_subscription(const QString& subscription) {
    return subscription.trimmed().compare("non-subscriber", Qt::CaseInsensitive) != 0;
}

QString normalize_call(const QString& text) {
    static const QRegularExpression ok("^[A-Z0-9]{1,4}[0-9][A-Z0-9]{0,4}(/[A-Z0-9]{1,4})?$");
    const QString up = text.trimmed().toUpper();
    return up.size() >= 3 && up.size() <= 14 && ok.match(up).hasMatch() ? up : QString();
}

static QUrl build(const QString& base, const QList<QPair<QString, QString>>& items) {
    QByteArray text = base.toLatin1() + '?';
    for (int i = 0; i < items.size(); ++i)
        text += (i ? "&" : "") + items[i].first.toLatin1() + '=' + QUrl::toPercentEncoding(items[i].second);
    return QUrl::fromEncoded(text, QUrl::StrictMode);
}

QUrl qrz_login_url(const QString& user, const QString& password) {
    return build("https://xmldata.qrz.com/xml/current/", {{"username", user}, {"password", password}, {"agent", kAgent}});
}
QUrl qrz_lookup_url(const QString& session, const QString& call) {
    return build("https://xmldata.qrz.com/xml/current/", {{"s", session}, {"callsign", call}, {"agent", kAgent}});
}
QUrl hamqth_login_url(const QString& user, const QString& password) {
    return build("https://www.hamqth.com/xml.php", {{"u", user}, {"p", password}});
}
QUrl hamqth_lookup_url(const QString& session, const QString& call) {
    return build("https://www.hamqth.com/xml.php", {{"id", session}, {"callsign", call}, {"prg", kAgent}});
}

} // namespace hamclock::lookup
