#include "lookup_model.hpp"
#include "../../ui/app_settings.hpp"
#include "../../domain/geometry.hpp"
#include "../../domain/locator_input.hpp"
#include <QCryptographicHash>
#include <QPointer>
#include <cmath>

namespace hamclock {
namespace {
constexpr int kTooManyRequests = 429;
constexpr int kFirstServerError = 500;

QString display_name(const QString& service) { return service == "qrz" ? "QRZ.com" : "HamQTH"; }

lookup::Reply parse_for(const QString& service, const QByteArray& body) {
    return service == "qrz" ? lookup::parse_qrz(body) : lookup::parse_hamqth(body);
}

// No answer, or a server too busy to answer: never reported as a bad password.
QString transport_problem(const FetchResult& res, const QString& service, QString* state = nullptr) {
    const auto set = [state](const char* s) { if (state) *state = s; };
    if (!res.error.isEmpty() && res.http_status == 0) { set("unreachable"); return "Lookup service unreachable"; }
    if (res.http_status == kTooManyRequests || res.http_status >= kFirstServerError) {
        set("busy");
        return QString("%1 is busy (HTTP %2); try again in a few minutes").arg(display_name(service)).arg(res.http_status);
    }
    return {};
}
} // namespace

LookupModel::LoginOutcome LookupModel::interpretLogin(const FetchResult& result, const QString& service) {
    LoginOutcome out;
    out.message = transport_problem(result, service, &out.state);
    if (!out.message.isEmpty()) return out;
    const auto reply = parse_for(service, result.body);
    using lookup::Outcome;
    if (reply.outcome == Outcome::Ok && !reply.session.isEmpty()) {
        const QString svc = display_name(service);
        out.session = reply.session;
        out.subscription = reply.subscription;
        if (!lookup::has_xml_subscription(reply.subscription)) {
            out.state = "limited";
            out.message = svc + " accepted the login, but this account has no XML subscription, so lookups return limited data";
        } else {
            out.state = "ok";
            out.message = reply.subscription.isEmpty() ? svc + " accepted the login"
                                                       : svc + " accepted the login (XML subscription until " + reply.subscription + ")";
        }
        return out;
    }
    if (reply.outcome == Outcome::LoginFailed) {
        out.state = "rejected";
        out.message = "Lookup login refused: check user and password";
    } else if (reply.outcome == Outcome::Blocked) {
        out.state = "blocked";
        out.message = "QRZ.com refused the connection and will not accept a login for 24 hours";
    } else {
        out.state = "failed";
        out.message = "Lookup login failed";
    }
    return out;
}

LookupModel::LookupModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("lookup", "Callsign lookup", parent), context_(context), settings_(context.settings) {
    if (settings_) connect(settings_, &AppSettings::accountsChanged, this, [this] {
        session_.clear();
        subscription_.clear();
        cache_.clear();
        current_.reset();
        message_.clear();
        dropStaleChecks();
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

QVariantMap LookupModel::checks() const {
    QVariantMap out;
    for (auto it = checks_.cbegin(); it != checks_.cend(); ++it)
        out.insert(it.key(), QVariantMap{{"state", it->state}, {"message", it->message}});
    return out;
}

// A digest only, so the password is not kept a second time in memory.
QByteArray LookupModel::accountDigest(const QString& service) const {
    if (!settings_) return {};
    const auto text = settings_->lookupUserFor(service) + '\n' + settings_->lookupPasswordFor(service);
    return QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256);
}

// A check stays valid while its service keeps the same user and password.
void LookupModel::dropStaleChecks() {
    bool dropped = false;
    for (auto it = checks_.begin(); it != checks_.end();) {
        if (it->account != accountDigest(it.key())) { it = checks_.erase(it); dropped = true; }
        else ++it;
    }
    if (dropped) emit checksChanged();
}

void LookupModel::record(const QString& service, const QString& state, const QString& message) {
    checks_.insert(service, {state, message, accountDigest(service)});
    emit checksChanged();
}

QString LookupModel::location() const {
    if (!current_) return {};
    return current_->qth.isEmpty() ? current_->country
           : current_->country.isEmpty() ? current_->qth : current_->qth + ", " + current_->country;
}

QString LookupModel::path() const {
    if (!current_ || !settings_ || !settings_->configured()) return {};
    return pathText(settings_->deLatitude(), settings_->deLongitude(), current_->grid);
}

QString LookupModel::pathText(double de_lat, double de_lon, const QString& grid) {
    const QString g = grid.trimmed();
    if (g.size() != 2 && g.size() != 4 && g.size() != 6) return {};
    try {
        Location dx = location_from_maidenhead(g.toStdString());
        // The decoder returns the south-west corner; aim at the middle of the cell.
        const double lon_cell = g.size() == 2 ? 20.0 : g.size() == 4 ? 2.0 : 5.0 / 60.0;
        const double lat_cell = g.size() == 2 ? 10.0 : g.size() == 4 ? 1.0 : 2.5 / 60.0;
        dx.longitude += lon_cell / 2;
        dx.latitude += lat_cell / 2;
        const GeometryResult r = geometry({de_lat, de_lon}, dx);
        if (r.status != BearingStatus::Defined) return QString("%1 km").arg(std::lround(r.distance_km));
        return QString("%1 km · %2° SP").arg(std::lround(r.distance_km)).arg(std::lround(r.short_bearing_deg) % 360);
    } catch (const std::exception&) {
        return {};
    }
}

void LookupModel::refreshState() {
    if (!configured()) { setState("unconfigured", "Sign in to HamQTH or QRZ.com in Settings > Accounts"); return; }
    const QString svc = display_name(settings_->lookupService());
    const QString limited = subscriptionWarning() ? " (no XML subscription: limited data)" : "";
    if (busy_) setState("loading", "Asking " + svc + "…");
    else if (current_) setState("live", "Lookup by " + svc + limited, now());
    else if (!message_.isEmpty()) setState("error", message_);
    else setState("live", "Enter a callsign (" + svc + ")" + limited);
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

void LookupModel::checkLogin(const QString& service) {
    if (!settings_ || (service != "qrz" && service != "hamqth") || !settings_->accountSignedIn(service)) return;
    if (checks_.value(service).state == "checking") return;
    record(service, "checking", "Checking " + display_name(service) + "…");
    const QString user = settings_->lookupUserFor(service), password = settings_->lookupPasswordFor(service);
    const QUrl url = service == "qrz" ? lookup::qrz_login_url(user, password) : lookup::hamqth_login_url(user, password);
    const QByteArray account = accountDigest(service);
    QPointer<LookupModel> self(this);
    context_.transport(url, [self, service, account](FetchResult res) {
        if (!self) return;
        // The account changed while the check was out: its answer no longer applies.
        if (self->accountDigest(service) != account) return;
        const auto outcome = interpretLogin(res, service);
        self->record(service, outcome.state, outcome.message);
        // A good check of the account lookups use also saves them a login.
        if (!outcome.session.isEmpty() && service == self->settings_->lookupService() && !self->busy_) {
            self->session_ = outcome.session;
            self->subscription_ = outcome.subscription;
            self->session_account_ = service + ":" + self->settings_->lookupUserFor(service);
            self->refreshState();
            emit self->resultChanged();
        }
    });
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
    const QString service = settings_->lookupService();
    const QString user = settings_->lookupUser(), password = settings_->lookupPassword();
    const QUrl url = service == "qrz" ? lookup::qrz_login_url(user, password) : lookup::hamqth_login_url(user, password);
    QPointer<LookupModel> self(this);
    context_.transport(url, [self, call, may_relogin, service](FetchResult res) {
        if (!self) return;
        const auto outcome = interpretLogin(res, service);
        // Settings > Accounts shows the latest word from the service, from a lookup too.
        self->record(service, outcome.state, outcome.message);
        if (outcome.session.isEmpty()) { self->fail(outcome.message); return; }
        self->session_ = outcome.session;
        self->subscription_ = outcome.subscription;
        self->session_account_ = service + ":" + self->settings_->lookupUser();
        self->query(call, may_relogin);
    });
}

void LookupModel::query(const QString& call, bool may_relogin) {
    const QString service = settings_->lookupService();
    const QUrl url = service == "qrz" ? lookup::qrz_lookup_url(session_, call) : lookup::hamqth_lookup_url(session_, call);
    QPointer<LookupModel> self(this);
    context_.transport(url, [self, call, may_relogin, service](FetchResult res) {
        if (!self) return;
        if (const QString problem = transport_problem(res, service); !problem.isEmpty()) {
            self->fail(problem);
            return;
        }
        const auto reply = parse_for(service, res.body);
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
