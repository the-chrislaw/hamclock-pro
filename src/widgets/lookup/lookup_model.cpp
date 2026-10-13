#include "lookup_model.hpp"
#include "../../ui/app_settings.hpp"
#include <QPointer>

namespace hamclock {

LookupModel::LookupModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("lookup", "Callsign lookup", parent), context_(context), settings_(context.settings) {
    if (settings_) connect(settings_, &AppSettings::accountsChanged, this, [this] {
        session_.clear();
        cache_.clear();
        current_.reset();
        message_.clear();
        refreshState();
        emit accountChanged();
        emit resultChanged();
    });
    refreshState();
}

bool LookupModel::configured() const {
    return settings_ && settings_->lookupService() != "none" && !settings_->lookupUser().isEmpty() &&
           settings_->hasLookupPassword();
}

QString LookupModel::location() const {
    if (!current_) return {};
    return current_->qth.isEmpty() ? current_->country
           : current_->country.isEmpty() ? current_->qth : current_->qth + ", " + current_->country;
}

void LookupModel::refreshState() {
    if (!configured()) { setState("unconfigured", "Add a QRZ.com or HamQTH account in Settings"); return; }
    const QString svc = settings_->lookupService() == "qrz" ? "QRZ.com" : "HamQTH";
    if (busy_) setState("loading", "Asking " + svc + "…");
    else if (current_) setState("live", "Lookup by " + svc, now());
    else if (!message_.isEmpty()) setState("error", message_);
    else setState("live", "Enter a callsign (" + svc + ")");
}

lookup::Reply LookupModel::parse(const QByteArray& body) const {
    return settings_->lookupService() == "qrz" ? lookup::parse_qrz(body) : lookup::parse_hamqth(body);
}

void LookupModel::lookup(const QString& text) {
    if (!configured() || busy_) return;
    const QString call = lookup::normalize_call(text);
    if (call.isEmpty()) { current_.reset(); message_ = "Not a callsign"; refreshState(); emit resultChanged(); return; }
    message_.clear();
    const auto hit = cache_.constFind(call);
    if (hit != cache_.constEnd()) { current_ = *hit; refreshState(); emit resultChanged(); return; }
    request(call, true);
}

void LookupModel::request(const QString& call, bool may_relogin) {
    busy_ = true;
    refreshState();
    emit resultChanged();
    // A session belongs to the account that opened it.
    const QString account = settings_->lookupService() + ":" + settings_->lookupUser();
    if (session_.isEmpty() || session_account_ != account) { session_.clear(); login(call, may_relogin); }
    else query(call, may_relogin);
}

void LookupModel::login(const QString& call, bool may_relogin) {
    const bool qrz = settings_->lookupService() == "qrz";
    const QString user = settings_->lookupUser(), password = settings_->lookupPassword();
    const QUrl url = qrz ? lookup::qrz_login_url(user, password) : lookup::hamqth_login_url(user, password);
    QPointer<LookupModel> self(this);
    context_.transport(url, [self, call, may_relogin](FetchResult res) {
        if (!self) return;
        if (!res.error.isEmpty() && res.http_status == 0) { self->fail("Lookup service unreachable"); return; }
        const auto reply = self->parse(res.body);
        if (reply.outcome != lookup::Outcome::Ok || reply.session.isEmpty()) {
            self->fail(reply.outcome == lookup::Outcome::LoginFailed ? "Lookup login refused: check user and password"
                                                                    : "Lookup login failed");
            return;
        }
        self->session_ = reply.session;
        self->session_account_ = self->settings_->lookupService() + ":" + self->settings_->lookupUser();
        self->query(call, may_relogin);
    });
}

void LookupModel::query(const QString& call, bool may_relogin) {
    const bool qrz = settings_->lookupService() == "qrz";
    const QUrl url = qrz ? lookup::qrz_lookup_url(session_, call) : lookup::hamqth_lookup_url(session_, call);
    QPointer<LookupModel> self(this);
    context_.transport(url, [self, call, may_relogin](FetchResult res) {
        if (!self) return;
        if (!res.error.isEmpty() && res.http_status == 0) { self->fail("Lookup service unreachable"); return; }
        const auto reply = self->parse(res.body);
        if (reply.outcome == lookup::Outcome::SessionExpired && may_relogin) {
            self->session_.clear();
            self->login(call, false);
            return;
        }
        self->finish(call, reply);
    });
}

void LookupModel::finish(const QString& call, const lookup::Reply& reply) {
    busy_ = false;
    using lookup::Outcome;
    if (reply.outcome == Outcome::Ok && reply.record) {
        cache_.insert(call, *reply.record);
        current_ = reply.record;
        message_.clear();
    } else {
        current_.reset();
        message_ = reply.outcome == Outcome::NotFound ? call + " not found" : "Lookup failed";
    }
    refreshState();
    emit resultChanged();
}

void LookupModel::fail(const QString& text) {
    busy_ = false;
    current_.reset();
    message_ = text;
    refreshState();
    emit resultChanged();
}

void register_widget_lookup(WidgetRegistry& registry, const WidgetContext& context) {
    registry.add({"lookup", "Callsign lookup", QUrl("qrc:/widgets/lookup/LookupCard.qml"), "strip", 60},
                 new LookupModel(context));
}

} // namespace hamclock
