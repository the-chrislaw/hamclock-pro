#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "lookup_parse.hpp"
#include <QHash>
#include <optional>

namespace hamclock {

class AppSettings;

// On-demand callsign lookup (QRZ XML or HamQTH) with the account from
// Settings. Logs in lazily, keeps the session key in memory only, logs in again
// once when the service reports the session expired, and caches answers for
// the run. The password is read from AppSettings when needed and is never put
// in a property, a status text or an error message.
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

    Q_INVOKABLE void lookup(const QString& call);
    // Settings "Test connection": logs in afresh (no lookup) and puts the outcome in message().
    Q_INVOKABLE void checkLogin();

signals:
    void accountChanged();
    void resultChanged();

private:
    enum class Step { Login, Query };
    void request(const QString& call, bool may_relogin);
    void login(const QString& call, bool may_relogin);
    void query(const QString& call, bool may_relogin);
    lookup::Reply parse(const QByteArray& body) const;
    void finish(const QString& call, const lookup::Reply& reply);
    void fail(const QString& text);
    void loginSucceeded();
    QString serviceName() const;
    void refreshState();

    WidgetContext context_;
    AppSettings* settings_;
    QString session_, session_account_;
    QHash<QString, lookup::Record> cache_;
    std::optional<lookup::Record> current_;
    QString message_;
    QString subscription_; // QRZ <SubExp> of the current session
    bool busy_ = false;
    bool error_ = false; // message_ reports a failure (not a check result)
};

void register_widget_lookup(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
