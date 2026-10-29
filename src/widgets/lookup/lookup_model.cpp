#include "lookup_model.hpp"
#include "../../ui/app_settings.hpp"
#include "../../domain/geometry.hpp"
#include "../../domain/locator_input.hpp"
#include <QPointer>
#include <cmath>

namespace hamclock {
namespace {
constexpr int kTooManyRequests = 429;
constexpr int kFirstServerError = 500;

// No answer, or a server too busy to answer: never reported as a bad password.
QString transport_problem(const FetchResult& res, const QString& service) {
    if (!res.error.isEmpty() && res.http_status == 0) return "Lookup service unreachable";
    if (res.http_status == kTooManyRequests || res.http_status >= kFirstServerError)
        return QString("%1 is busy (HTTP %2); try again in a few minutes").arg(service).arg(res.http_status);
    return {};
}
} // namespace

LookupModel::LookupModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("lookup", "Callsign lookup", parent), context_(context), settings_(context.settings) {
    if (settings_) connect(settings_, &AppSettings::accountsChanged, this, [this] {
        session_.clear();
        subscription_.clear();
        cache_.clear();
        current_.reset();
        message_.clear();
        error_ = false;
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
    if (!configured()) { setState("unconfigured", "Add a QRZ.com or HamQTH account in Settings"); return; }
    const QString svc = serviceName();
    const QString limited = subscriptionWarning() ? " (no XML subscription: limited data)" : "";
    if (busy_) setState("loading", "Asking " + svc + "…");
    else if (current_) setState("live", "Lookup by " + svc + limited, now());
    else if (!message_.isEmpty() && error_) setState("error", message_);
    else setState("live", "Enter a callsign (" + svc + ")" + limited);
}

QString LookupModel::serviceName() const {
    return settings_ && settings_->lookupService() == "qrz" ? "QRZ.com" : "HamQTH";
}

lookup::Reply LookupModel::parse(const QByteArray& body) const {
    return settings_->lookupService() == "qrz" ? lookup::parse_qrz(body) : lookup::parse_hamqth(body);
}

void LookupModel::lookup(const QString& text) {
    if (!configured() || busy_) return;
    const QString call = lookup::normalize_call(text);
    if (call.isEmpty()) { current_.reset(); message_ = "Not a callsign"; error_ = true; refreshState(); emit resultChanged(); return; }
    message_.clear();
    error_ = false;
    const auto hit = cache_.constFind(call);
    if (hit != cache_.constEnd()) { current_ = *hit; refreshState(); emit resultChanged(); return; }
    request(call, true);
}

void LookupModel::checkLogin() {
    if (!configured() || busy_) return;
    current_.reset();
    message_.clear();
    error_ = false;
    busy_ = true;
    session_.clear();
    refreshState();
    emit resultChanged();
    login({}, false);
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
        if (const QString problem = transport_problem(res, self->serviceName()); !problem.isEmpty()) {
            self->fail(problem);
            return;
        }
        const auto reply = self->parse(res.body);
        if (reply.outcome != lookup::Outcome::Ok || reply.session.isEmpty()) {
            using lookup::Outcome;
            self->fail(reply.outcome == Outcome::LoginFailed ? "Lookup login refused: check user and password"
                       : reply.outcome == Outcome::Blocked
                           ? "QRZ.com refused the connection and will not accept a login for 24 hours"
                           : "Lookup login failed");
            return;
        }
        self->session_ = reply.session;
        self->subscription_ = reply.subscription;
        self->session_account_ = self->settings_->lookupService() + ":" + self->settings_->lookupUser();
        if (call.isEmpty()) self->loginSucceeded();
        else self->query(call, may_relogin);
    });
}

void LookupModel::loginSucceeded() {
    busy_ = false;
    error_ = false;
    const QString svc = serviceName();
    if (subscriptionWarning())
        message_ = svc + " accepted the login, but this account has no XML subscription, so lookups return limited data";
    else if (!subscription_.isEmpty())
        message_ = svc + " accepted the login (XML subscription until " + subscription_ + ")";
    else
        message_ = svc + " accepted the login";
    refreshState();
    emit resultChanged();
}

void LookupModel::query(const QString& call, bool may_relogin) {
    const bool qrz = settings_->lookupService() == "qrz";
    const QUrl url = qrz ? lookup::qrz_lookup_url(session_, call) : lookup::hamqth_lookup_url(session_, call);
    QPointer<LookupModel> self(this);
    context_.transport(url, [self, call, may_relogin](FetchResult res) {
        if (!self) return;
        if (const QString problem = transport_problem(res, self->serviceName()); !problem.isEmpty()) {
            self->fail(problem);
            return;
        }
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
        error_ = false;
    } else {
        current_.reset();
        message_ = reply.outcome == Outcome::NotFound ? call + " not found" : "Lookup failed";
        error_ = true;
    }
    refreshState();
    emit resultChanged();
}

void LookupModel::fail(const QString& text) {
    busy_ = false;
    current_.reset();
    message_ = text;
    error_ = true;
    refreshState();
    emit resultChanged();
}

void register_widget_lookup(WidgetRegistry& registry, const WidgetContext& context) {
    registry.add({"lookup", "Callsign lookup", QUrl("qrc:/widgets/lookup/LookupCard.qml"), "strip", 60},
                 new LookupModel(context));
}

} // namespace hamclock
