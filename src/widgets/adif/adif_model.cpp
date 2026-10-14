#include "adif_model.hpp"
#include "../dxcluster/dxcc_table.hpp"
#include "../../ui/app_settings.hpp"
#include <QFile>
#include <QFileInfo>
#include <algorithm>

namespace hamclock {

namespace { constexpr qint64 kMaxFileBytes = 64LL * 1024 * 1024; }

AdifModel::AdifModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("adif", "Log", parent), settings_(context.settings) {
    connect(&watcher_, &QFileSystemWatcher::fileChanged, this, [this] { reload(); });
    if (settings_) connect(settings_, &AppSettings::optionsChanged, this, &AdifModel::reload);
}

void AdifModel::start() { reload(); }

void AdifModel::reload() {
    load(settings_ ? settings_->adifPath() : QString());
}

void AdifModel::load(const QString& path) {
    if (path != path_) {
        if (!path_.isEmpty()) watcher_.removePath(path_);
        path_ = path;
    }
    qsos_.clear();
    calls_.clear();
    entities_.clear();
    slots_.clear();
    loaded_ = false;
    if (path.isEmpty()) {
        setState("unconfigured", "Set an ADIF log file in Settings");
        emit logChanged();
        return;
    }
    if (!watcher_.files().contains(path)) watcher_.addPath(path);  // editors that replace the file re-add it here
    QFile file(path);
    if (!file.exists() || QFileInfo(file).size() > kMaxFileBytes || !file.open(QIODevice::ReadOnly)) {
        setState("error", file.exists() ? "Cannot read the ADIF log" : "ADIF log file not found");
        emit logChanged();
        return;
    }
    qsos_ = adif::parse(QString::fromUtf8(file.readAll()));
    std::stable_sort(qsos_.begin(), qsos_.end(), [](const adif::Qso& a, const adif::Qso& b) { return a.time > b.time; });
    for (const auto& q : qsos_) {
        calls_.insert(adif::base_call(q.call));
        const QString e = entityFor(q.call);
        if (e.isEmpty()) continue;
        entities_.insert(e);
        if (!q.band.isEmpty()) slots_.insert(e + '|' + q.band);
    }
    loaded_ = true;
    setState("live", QString("%1 QSOs in log").arg(qsos_.size()), QFileInfo(file).lastModified().toUTC());
    emit logChanged();
}

bool AdifModel::worked(const QString& call) const { return calls_.contains(adif::base_call(call)); }

QString AdifModel::entityFor(const QString& call) const {
    if (entity_of_) return entity_of_(call).toUpper();
    const auto* e = dxcluster::DxccTable::bundled().lookup(call);
    return e ? e->primary_prefix.toUpper() : QString();
}

void AdifModel::setEntityResolver(std::function<QString(const QString&)> resolver) {
    entity_of_ = std::move(resolver);
    if (loaded_) load(path_);
}

bool AdifModel::entityNeeded(const QString& call) const {
    if (!loaded_) return false;
    const QString e = entityFor(call);
    return !e.isEmpty() && !entities_.contains(e);
}

bool AdifModel::bandSlotNeeded(const QString& call, const QString& band) const {
    if (!loaded_ || band.isEmpty()) return false;
    const QString e = entityFor(call);
    return !e.isEmpty() && !slots_.contains(e + '|' + band.toLower());
}

QVariantList AdifModel::recent() const {
    QVariantList out;
    for (size_t i = 0; i < qsos_.size() && i < 8; ++i) {
        const auto& q = qsos_[i];
        out.push_back(QVariantMap{{"call", q.call}, {"band", q.band}, {"mode", q.mode}, {"grid", q.grid},
                                  {"age", q.time.isValid() ? formatAge(q.time, now()) : QString()}});
    }
    return out;
}

void register_widget_adif(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new AdifModel(context);
    registry.add({"adif", "Log", QUrl("qrc:/widgets/adif/AdifCard.qml"), "side", 140}, model);
    QTimer::singleShot(0, model, &AdifModel::start);
}

} // namespace hamclock
