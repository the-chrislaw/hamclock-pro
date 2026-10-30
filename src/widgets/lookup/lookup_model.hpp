#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "lookup_parse.hpp"
#include <QHash>
#include <QVariantMap>
#include <optional>

namespace hamclock {

class AppSettings;

// On-demand callsign lookup (QRZ XML or HamQTH) with the account from
// Settings > Accounts (QRZ.com when signed in there, else HamQTH). Logs in lazily,
// keeps the session key in memory only, logs in again once when the service reports
// the session expired, and caches answers for the run. The password is read from
// AppSettings when needed and is never put in a property, a status text or a message.
class LookupModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured NOTIFY accountChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY resultChanged)
    Q_PROPERTY(QString call READ call NOTIFY resultChanged)
    Q_PROPERTY(QString name READ name NOTIFY resultChanged)
    Q_PROPERTY(QString location READ location NOTIFY resultChanged)
    Q_PROPERTY(QString grid READ grid NOTIFY resultChanged)
    Q_PROPERTY(QString country READ country NOTIFY resultChanged)
    Q_PROPERTY(QString message READ message NOTIFY resultChanged)
    Q_PROPERTY(QString path READ path NOTIFY resultChanged)
    // True after a QRZ login reported "non-subscriber": lookups work but return a reduced record.
    Q_PROPERTY(bool subscriptionWarning READ subscriptionWarning NOTIFY resultChanged)
    // Sign-in checks for Settings > Accounts: {"qrz"|"hamqth": {state, message}}. state is
    // checking, ok, limited (QRZ without XML), rejected, blocked, busy, unreachable or failed.
    Q_PROPERTY(QVariantMap checks READ checks NOTIFY checksChanged)
public:
    explicit LookupModel(const WidgetContext& context, QObject* parent = nullptr);

    bool configured() const;
    bool busy() const { return busy_; }
    QString call() const { return current_ ? current_->call : QString(); }
    QString name() const { return current_ ? current_->name : QString(); }
    QString location() const;
    QString grid() const { return current_ ? current_->grid : QString(); }
    QString country() const { return current_ ? current_->country : QString(); }
    QString message() const { return message_; }
    // "5210 km · 52° SP" from DE to the looked-up grid centre; empty without a DE or a usable grid.
    QString path() const;
    static QString pathText(double de_lat, double de_lon, const QString& grid);
    bool subscriptionWarning() const { return !lookup::has_xml_subscription(subscription_); }
    QVariantMap checks() const;

    Q_INVOKABLE void lookup(const QString& call);
    // Logs in to `service` ("qrz" or "hamqth") with its saved account, without a lookup,
    // and records the outcome in checks(). Does nothing for a service that is not signed in.
    Q_INVOKABLE void checkLogin(const QString& service);

    // What a login reply means, for both lookups and checks.
    struct LoginOutcome {
        QString state;    // as in checks()
        QString message;  // user-facing; never the password or the request URL
        QString session, subscription;
    };
    static LoginOutcome interpretLogin(const FetchResult& result, const QString& service);

signals:
    void accountChanged();
    void resultChanged();
    void checksChanged();

private:
    struct Check {
        QString state, message;
        QByteArray account;  // digest of the user and password checked
    };
    void request(const QString& call, bool may_relogin);
    void login(const QString& call, bool may_relogin);
    void query(const QString& call, bool may_relogin);
    void finish(const QString& call, const lookup::Reply& reply);
    void fail(const QString& text);
    void record(const QString& service, const QString& state, const QString& message);
    void dropStaleChecks();
    QByteArray accountDigest(const QString& service) const;
    void refreshState();

    WidgetContext context_;
    AppSettings* settings_;
    QString session_, session_account_;
    QHash<QString, lookup::Record> cache_;
    std::optional<lookup::Record> current_;
    QString message_;
    QString subscription_;  // QRZ <SubExp> of the current session
    QHash<QString, Check> checks_;
    bool busy_ = false;
};

void register_widget_lookup(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
