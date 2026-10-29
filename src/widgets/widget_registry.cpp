#include "widget_registry.hpp"
#include <QStringList>
#include <algorithm>
#include <stdexcept>

namespace hamclock {

bool widget_available(const QString& id, bool ios) {
    static const QStringList kLocalStationOnly{"radio", "rotator", "gps", "adsb", "sensors", "adif"};
    return !ios || !kLocalStationOnly.contains(id);
}

WidgetRegistry::WidgetRegistry(QObject* parent) : QAbstractListModel(parent) {}

bool WidgetRegistry::add(WidgetInfo info, WidgetModel* model) {
    // The registry owns the model even when it rejects it, so callers never leak.
    if (!model || info.id.isEmpty() || !info.card.isValid()) {
        delete model;
        throw std::invalid_argument("Incomplete widget");
    }
    if (this->model(info.id)) {
        delete model;
        throw std::invalid_argument("Duplicate widget id");
    }
#ifdef Q_OS_IOS
    constexpr bool kIos = true;
#else
    constexpr bool kIos = false;
#endif
    model->setParent(this);
    // Kept alive (registration code may still use it) but never listed or looked up.
    if (!widget_available(info.id, kIos)) return false;
    const auto position = std::upper_bound(entries_.begin(), entries_.end(), info.order,
        [](int order, const Entry& entry) { return order < entry.info.order; });
    const int row = static_cast<int>(position - entries_.begin());
    beginInsertRows({}, row, row);
    entries_.insert(position, Entry{std::move(info), model});
    endInsertRows();
    emit countChanged();
    return true;
}

int WidgetRegistry::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(entries_.size());
}

QVariant WidgetRegistry::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
    const auto& entry = entries_[static_cast<std::size_t>(index.row())];
    switch (role) {
    case IdRole: return entry.info.id;
    case TitleRole: return entry.info.title;
    case CardRole: return entry.info.card;
    case AreaRole: return entry.info.area;
    case ModelRole: return QVariant::fromValue(static_cast<QObject*>(entry.model));
    default: return {};
    }
}

QHash<int, QByteArray> WidgetRegistry::roleNames() const {
    return {{IdRole, "widgetId"}, {TitleRole, "title"}, {CardRole, "card"},
            {AreaRole, "area"}, {ModelRole, "widgetModel"}};
}

QObject* WidgetRegistry::model(const QString& id) const {
    for (const auto& entry : entries_)
        if (entry.info.id == id) return entry.model;
    return nullptr;
}

QStringList WidgetRegistry::ids() const {
    QStringList out;
    for (const auto& entry : entries_) out << entry.info.id;
    return out;
}

} // namespace hamclock
