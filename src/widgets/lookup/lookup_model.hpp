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

    Q_INVOKABLE void lookup(const QString& call);

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
    void refreshState();

    WidgetContext context_;
    AppSettings* settings_;
    QString session_, session_account_;
    QHash<QString, lookup::Record> cache_;
    std::optional<lookup::Record> current_;
    QString message_;
    bool busy_ = false;
};

void register_widget_lookup(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
